// RequestParser.cpp

#include <sstream>

#include "Enums.hpp"
#include "Request.hpp"
#include "RequestParser.hpp"
#include "Utils.hpp"
#include "Webserv.hpp"

RequestParser::RequestParser(void)
    : _parserState(REQ_PARSE_START), _parsingPhase(PARSING_START_LINE), _accumulator(), _firstSection(), _startLine(),
      _headersBuffer(), _contentLength(-1), _chunkSize(-1), _maxBodySize(DEFAULT_MAX_BODY_SIZE), _bodyBuffer() {
}

RequestParser::~RequestParser(void) {
}

void RequestParser::resetParser(void) {
    _parserState   = REQ_PARSE_START;
    _parsingPhase  = PARSING_START_LINE;
    _contentLength = -1;
    _firstSection.clear();
    _startLine.clear();
    _headersBuffer.clear();
    _bodyBuffer.clear();
    _chunkSize = -1;
}

ParserState RequestParser::getState(void) const {
    return _parserState;
}

void RequestParser::setState(ParserState parserState) {
    _parserState = parserState;
}

void RequestParser::handleParseError(Request& req, std::queue<Request>& reqQueue, const char* msg) {
    DEBUG_LOG("Parse error: " + std::string(msg));
    (void)msg;
    if (req.getStatusCode() == NO_STATUS)
        req.setStatusCode(BAD_REQUEST);
    reqQueue.push(req);
    _parserState = REQ_PARSE_ERROR;
}

// extracts request-line and header from accumulator
int RequestParser::extractFirstSection(void) {
    size_t pos;
    pos = _accumulator.find(CRLF + CRLF);
    if (pos == std::string::npos) {
        _firstSection += _accumulator;
        if (_firstSection.size() >= READ_BUF_SIZE)
            throw RequestParsingError("first section (request-line + headers) too long");
        _accumulator.clear();
        _parserState = REQ_PARSE_PARTIAL;
        return READ_MORE;
    } else {
        _firstSection += _accumulator.substr(0, pos);
        if (_firstSection.size() >= READ_BUF_SIZE || _firstSection.size() < MIN_REQ_SIZE)
            throw RequestParsingError("first section (request-line + headers) too long or too short");
        _accumulator = _accumulator.substr(pos + 4);
    }
    return FIRST_SECTION_OK;
}

void RequestParser::extractStartLineFromFirstSection(void) {
    size_t pos;

    pos = _firstSection.find(CRLF);
    if (pos != std::string::npos) { // _firstSection has start-line + headers
        _startLine    = _firstSection.substr(0, pos);
        _firstSection = _firstSection.substr(pos + 2);
    } else // _firstSection is a pure start-line with no headers
        throw RequestParsingError("parsing start line: no headers found");
}

// reminder: maxBodySize is checked previously in PARSING_HEADERS => we use _contentLength
int RequestParser::extractFullBody(void) {
    size_t lenToAdd; // length to add to buffer

    if (_accumulator.empty()) {
        _parserState = REQ_PARSE_PARTIAL;
        return READ_MORE;
    }
    if (_contentLength == -1)
        throw RequestParsingError("extractFullBody: _contentLength was not set");
    if (_contentLength == 0)
        return CONTENT_LENGTH_OK;
    if (static_cast<int>(_bodyBuffer.size()) > _contentLength)
        throw RequestParsingError("body: request parser extracted more bytes than _contentLength from read() buffer");
    lenToAdd = _contentLength -
               _bodyBuffer.size(); // we try to take as much as possible so that _bodyBuffer.size() == _contentLength
    if (_accumulator.size() < lenToAdd) // in case the accumulator doesn't have enough, we only take what's there
        lenToAdd = _accumulator.size();

    _bodyBuffer += _accumulator.substr(0, lenToAdd);
    _accumulator = _accumulator.substr(lenToAdd);

    if (static_cast<int>(_bodyBuffer.size()) < _contentLength) {
        _parserState = REQ_PARSE_PARTIAL;
        return READ_MORE;
    }
    return CONTENT_LENGTH_OK;
}

// extracts chunk size in hex and stores it in _chunkSize
int RequestParser::extractChunkSize(void) {
    size_t      pos;
    size_t      chunkSize;
    std::string chunkSizeStr;

    pos          = _accumulator.find(CRLF);
    chunkSizeStr = _accumulator.substr(0, pos);

    if (chunkSizeStr.size() > MAX_CHUNK_SIZE_LINE_SIZE)
        throw RequestParsingError("chunked input: chunk size too big");
    else if (chunkSizeStr.find_first_not_of("0123456789abcdefABCDEF") != std::string::npos)
        throw RequestParsingError("chunked input: chunk size contains non-numeric characters");
    else if (pos == std::string::npos) {
        _parserState = REQ_PARSE_PARTIAL;
        return READ_MORE;
    } else {
        std::istringstream(chunkSizeStr) >> std::hex >> chunkSize;
        if (chunkSize > MAX_CHUNK_SIZE)
            throw RequestParsingError("chunked input: chunk size too big");
        else if (_bodyBuffer.size() + chunkSize > static_cast<size_t>(_maxBodySize))
            throw RequestParsingError("chunked input: parsed body would be too large (> " + toString(_maxBodySize) +
                                      " bytes).\n");
        else {
            _chunkSize   = chunkSize;
            _accumulator = _accumulator.substr(pos + 2);
        }
    }
    return CHUNK_SIZE_OK;
}

int RequestParser::extractChunkData(void) {
    std::string chunkDataStr;

    if (_chunkSize == 0) {
        if (_accumulator[0] != '\r' || _accumulator[1] != '\n')
            throw RequestParsingError("extractChunkData: 0CRLF isn't followed by CRLF");
        _accumulator = _accumulator.substr(2);
        return CHUNK_FINISHED;
    }
    if (static_cast<int>(_accumulator.size()) < _chunkSize + 2) {
        _parserState = REQ_PARSE_PARTIAL;
        return READ_MORE;
    }
    if (_accumulator[_chunkSize] != '\r' || _accumulator[_chunkSize + 1] != '\n')
        throw RequestParsingError("chunk data isn't followed by CRLF");

    chunkDataStr = _accumulator.substr(0, _chunkSize);
    _accumulator = _accumulator.substr(_chunkSize + 2);
    _bodyBuffer += chunkDataStr;
    DEBUG_LOG("extractChunkData - chunkDataStr: {" + chunkDataStr + "}, accumulator: {" + _accumulator +
              "}, bodyBuffer: {" + _bodyBuffer + "}");
    return PARSE_MORE_CHUNKS;
}

void RequestParser::feed(char* buf, std::queue<Request>& reqQueue) {
    Request req;
    int     ret;

    _accumulator += buf;

    try {
        while (!_accumulator.empty()) {
            // 1. extract content from the accumulator
            switch (_parsingPhase) {
                case PARSING_START_LINE:
                case PARSING_HEADERS:
                    ret = extractFirstSection();
                    if (ret == READ_MORE)
                        return;
                    break;
                case PARSING_BODY_CONTENT_LENGTH:
                    ret = extractFullBody();
                    if (ret == READ_MORE)
                        return;
                    _parsingPhase = PARSING_BODY_FINISHED;
                    break;
                case PARSING_BODY_CHUNKED:
                    ret = extractChunkSize();
                    if (ret == CHUNK_SIZE_OK)
                        ret = extractChunkData();
                    if (ret == READ_MORE)
                        return;
                    if (ret == PARSE_MORE_CHUNKS)
                        continue;
                    _parsingPhase = PARSING_BODY_FINISHED;
                    break;
                case PARSING_COMPLETE:
                    break;
                default:
                    break;
            }

            // 2. parse the extracted content
            if (_parsingPhase == PARSING_START_LINE) {
                extractStartLineFromFirstSection();
                parseStartLine(req);
                _parsingPhase = PARSING_HEADERS;
            }
            if (_parsingPhase == PARSING_HEADERS) {
                _headersBuffer = _firstSection;
                parseHeaders(req);
                if (req.hasHeader(CONTENT_LENGTH) && _contentLength != 0) {
                    _parsingPhase = PARSING_BODY_CONTENT_LENGTH;
                    continue;
                }
                if (req.hasHeader(TRANSFER_ENCODING)) {
                    _parsingPhase = PARSING_BODY_CHUNKED;
                    continue;
                } else
                    _parsingPhase = PARSING_COMPLETE;
            }
            if (_parsingPhase == PARSING_BODY_FINISHED) {
                DEBUG_LOG("PARSING_BODY_FINISHED; _bodyBuffer.size(): " + toString(_bodyBuffer.size()));
                req.setBody(_bodyBuffer);
                _parsingPhase = PARSING_COMPLETE;
            }
            if (_parsingPhase == PARSING_COMPLETE) {
                DEBUG_LOG("PARSING_COMPLETE");
                if (_accumulator.empty()) {
                    DEBUG_LOG("accumulator empty: OK");
                } else {
                    DEBUG_LOG("PROBLEM: accumulator not empty: {" + _accumulator + "}" +
                              " size: " + toString(_accumulator.size()));
                }
                DEBUG_LOG(req);
                reqQueue.push(req);
                req = Request();
                this->resetParser();
                _parsingPhase = PARSING_START_LINE;
            }
        }
    } catch (RequestParsingError& e) {
        return handleParseError(req, reqQueue, e.what());
    }
    DEBUG_LOG("end of feed()");
    _parserState = REQ_PARSE_COMPLETE;
}
