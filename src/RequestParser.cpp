// RequestParser.cpp

#include "RequestParser.hpp"
#include "Enums.hpp"
#include "Request.hpp"
#include "Utils.hpp"
#include "Webserv.hpp"

RequestParser::RequestParser(void)
    : _parserState(REQ_PARSE_START), _parsingPhase(PARSING_START_LINE), _contentLength(0), _accumulator(),
      _firstSection(), _startLine(), _headersBuffer(), _bodyBuffer() {
}

RequestParser::~RequestParser(void) {
}

void RequestParser::resetParser(void) {
    _parserState   = REQ_PARSE_START;
    _parsingPhase  = PARSING_START_LINE;
    _contentLength = 0;
    _firstSection.clear();
    _startLine.clear();
    _headersBuffer.clear();
    _bodyBuffer.clear();
}

ParserState RequestParser::getState(void) {
    return _parserState;
}

void RequestParser::setState(ParserState parserState) {
    _parserState = parserState;
}

void RequestParser::handleParseError(Request& req, std::queue<Request>& reqQueue, const char* msg) {
    DEBUG_LOG("Parse error: " + msg);
    (void)msg;
    if (req.getStatusCode() == NO_STATUS)
        req.setStatusCode(BAD_REQUEST);
    reqQueue.push(req);
    _parserState = REQ_PARSE_ERROR;
}

void RequestParser::extractStartLineFromFirstSection(void) {
    size_t pos;

    pos = _firstSection.find(CRLF);
    if (pos != std::string::npos) { // _firstSection has start-line + headers
        _startLine    = _firstSection.substr(0, pos);
        _firstSection = _firstSection.substr(pos + 2);
    } else { // _firstSection is a pure start-line with no headers
        DEBUG_LOG("exiting at PARSING_START_LINE: no headers found");
        throw RequestParsingError("parsing start line: no headers found");
    }
}

int RequestParser::extractFullBody(size_t maxBodySize) {
    // reminder: maxBodySize is checked previously in PARSING_HEADERS
    size_t lenToAdd;

    lenToAdd = _contentLength - _bodyBuffer.size(); // needed for _bodyBuffer.size() == _contentLength,
    if (_accumulator.size() < lenToAdd) // if the accumulator doesn't have enough, we take what's there
        lenToAdd = _accumulator.size();

    _bodyBuffer += _accumulator.substr(0, lenToAdd);
    _accumulator = _accumulator.substr(lenToAdd);

    if (_bodyBuffer.size() < _contentLength)
        return READ_MORE;

        // any error cases to handle ?

    return FULL_BODY_OK;
}

#include <sstream>
#include <ostream>

// extracts chunk size in hex and stores it in _chunkSize
int RequestParser::extractChunkSize(size_t maxBodySize) {
    size_t      pos;
    size_t      chunkSize;
    std::string chunkSizeStr;

    pos = _accumulator.find(CRLF);
    chunkSizeStr = _accumulator.substr(0, pos);

    if (chunkSizeStr.size() > MAX_CHUNK_SIZE_LINE_SIZE)
        throw RequestParsingError("chunked input: chunk size too big");
    else if (chunkSizeStr.find_first_not_of("0123456789abcdefABCDEF") != std::string::npos)
        throw RequestParsingError("chunked input: chunk size contains non-numeric characters");
    else if (pos == std::string::npos)
        return READ_MORE;
    else {
        std::istringstream(chunkSizeStr) >> std::hex >> chunkSize;
        if (chunkSize > maxBodySize)
            throw RequestParsingError("chunked input: chunk size too big");
        else
        {
             _chunkSize = chunkSize;
            _accumulator = _accumulator.substr(pos + 2);
        }
    }
    return CHUNK_SIZE_OK;
}

// extracts request-line and header from accumulator
int RequestParser::extractFirstSection(void) {
    size_t pos;
    pos = _accumulator.find(CRLF + CRLF);
    if (pos == std::string::npos) {
        _firstSection += _accumulator; // .substr(0, pos)
        if (_firstSection.size() >= READ_BUF_SIZE)
            throw RequestParsingError("first section (request-line + headers) too long");
        _accumulator.clear();
        _parserState = REQ_PARSE_PARTIAL;
        return READ_MORE;
    } else {
        _firstSection += _accumulator.substr(0, pos);
        if (_firstSection.size() >= READ_BUF_SIZE || _firstSection.size() < MIN_REQ_SIZE) {
            throw RequestParsingError("first section (request-line + headers) too long or too short");
        }
        _accumulator = _accumulator.substr(pos + 4);
    }
    return FIRST_SECTION_OK;
}

int RequestParser::extractChunkData(void) {
    size_t      pos;
    size_t      chunkData;
    std::string chunkDataStr;

    if (_chunkSize == 0)
    {
        // finished (see pseudocode)
    }
    if (_accumulator.size() < _chunkSize + 2)
        return READ_MORE;
    if (_accumulator[_chunkSize] != '\r' || _accumulator[_chunkSize + 1] != '\n')
        throw RequestParsingError("chunk data isn't followed by CRLF");


    chunkDataStr = _accumulator.substr(0, pos); // if no CRLF found => whole line;


    /* get a line:
        if no line found 
    
    
    */

    chunkDataStr = _accumulator.substr(0, pos);
    _bodyBuffer += chunkData;
}


void RequestParser::feed(char* buf, std::queue<Request>& reqQueue, size_t maxBodySize) {
    Request req;
    int     ret;

    _accumulator += buf;
    
    try {
    while (!_accumulator.empty()) {
        /* 1. extract the content from the accumulator => _firstSection or _bodyBuffer */
        switch (_parsingPhase) {
            case PARSING_START_LINE:
            case PARSING_HEADERS:
                ret = extractFirstSection();
                if (ret == READ_MORE)
                    return;
                break;
            case PARSING_FULL_BODY:
                ret = extractFullBody(maxBodySize);
                if (ret == READ_MORE)
                    return;
                break;
            case PARSING_CHUNKED:
                ret = extractChunkSize(maxBodySize);
                if (ret == CHUNK_SIZE_OK)
                    extractChunkData();
                else if (ret == READ_MORE)
                    return;
                break;
            case PARSING_COMPLETE:
                break;
            default:
                break;
    }

    /* 2. parse the extracted content */
        if (_parsingPhase == PARSING_START_LINE) // internal
        {
            extractStartLineFromFirstSection();
            parseStartLine(req);
            _parsingPhase = PARSING_HEADERS;
        }
        if (_parsingPhase == PARSING_HEADERS) {
            _headersBuffer = _firstSection;
            parseHeaders(req, maxBodySize);
            DEBUG_LOG(req);
            if (req.hasHeader(CONTENT_LENGTH)) {
                DEBUG_LOG("PARSING_HEADERS: found header Content-length");
                _parsingPhase = PARSING_FULL_BODY;
                continue;
            }
            if (req.hasHeader(TRANSFER_ENCODING)) {
                DEBUG_LOG("PARSING_HEADERS: found header Transfer-encoding");
                _parsingPhase = PARSING_CHUNKED;
                continue;
            } else {
                DEBUG_LOG("PARSING_HEADERS: didn't find header Content-length or Transfer-encoding, parsing complete");
                _parsingPhase = PARSING_COMPLETE;
            }
        }
        if (_parsingPhase == PARSING_FULL_BODY) {
            DEBUG_LOG("_bodyBuffer.size(): " + toString(_bodyBuffer.size()));
            parseFullBody(req, maxBodySize);
            DEBUG_LOG("PARSING_BODY");
            _parsingPhase = PARSING_COMPLETE;
        }
        if (_parsingPhase == PARSING_CHUNKED) {
            // _parsingPhase = PARSING_COMPLETE;
        }
        if (_parsingPhase == PARSING_COMPLETE) {
            DEBUG_LOG("PARSING_COMPLETE");
            if (_accumulator.empty())
                DEBUG_LOG("accumulator empty");
            else
                DEBUG_LOG("accumulator not empty: {" + _accumulator + "}");
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

bool RequestParser::isValidBody(Request& req) const { // not called yet
    // move validation of content length == body.size
    if (req.getMethod() == POST && req.getBody().empty())
        return false;

    if (req.getMethod() == GET && !req.getBody().empty())
        return false;

    if (req.getBody().size() != toSizet(req.getHeader(CONTENT_LENGTH))) {
        DEBUG_LOG("req.getBody().size(): " + toString(req.getBody().size()) + " != content-length(" +
                  toString(toSizet(req.getHeader(CONTENT_LENGTH))));
        return false;
    }
    return true;
}