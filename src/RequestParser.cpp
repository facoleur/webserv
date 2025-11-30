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

void RequestParser::handleParseError(Request& req, std::queue<Request>& reqQueue) {
    DEBUG_LOG("Parse error");
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

void RequestParser::feed(char* buf, std::queue<Request>& reqQueue, size_t maxBodySize) {
    size_t  pos;
    size_t  lenToAdd;
    Request req;

    _accumulator += buf;
    while (!_accumulator.empty()) {
        /* 1. extract the content from the accumulator => _firstSection or _bodyBuffer */
        switch (_parsingPhase) {
            case PARSING_START_LINE:
            case PARSING_HEADERS:
                pos = _accumulator.find(CRLF + CRLF);
                if (pos == std::string::npos) {
                    _firstSection += _accumulator; // .substr(0, pos)
                    if (_firstSection.size() >= READ_BUF_SIZE)
                        return handleParseError(req, reqQueue);
                    _accumulator.clear();
                    _parserState = REQ_PARSE_PARTIAL;
                    return;
                } else {
                    _firstSection += _accumulator.substr(0, pos);
                    if (_firstSection.size() >= READ_BUF_SIZE || _firstSection.size() < MIN_REQ_SIZE) {
                        return handleParseError(req, reqQueue);
                    }
                    _accumulator = _accumulator.substr(pos + 4);
                }
                break;
            case PARSING_BODY: // if (REQ_PARSE_CHUNK) ? // if (REQ_PARSE_FULL_BODY) ?
                // // reminder: maxBodySize is checked previously in PARSING_HEADERS
                lenToAdd = _contentLength - _bodyBuffer.size(); // needed for _bodyBuffer.size() == _contentLength,
                if (_accumulator.size() < lenToAdd) // if the accumulator doesn't have enough, we take what's there
                    lenToAdd = _accumulator.size();

                DEBUG_LOG("adding to {" + _accumulator.substr(0, lenToAdd) + "} to _bodyBuffer");
                _bodyBuffer += _accumulator.substr(0, lenToAdd);
                DEBUG_LOG("_bodyBuffer is now {" + _bodyBuffer + "}");
                _accumulator = _accumulator.substr(lenToAdd);
                DEBUG_LOG("and _accumulator is now {" + _accumulator + "}");

                if (_bodyBuffer.size() < _contentLength) {
                    _parserState = REQ_PARSE_PARTIAL;
                    return;
                }
                break;
            case PARSING_COMPLETE:
                break;
            default:
                break;
        }

        /* 2. parse the extracted content */
        try {
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
                if (req.hasHeader(CONTENT_LENGTH) || req.hasHeader(TRANSFER_ENCODING)) {
                    DEBUG_LOG("PARSING_HEADERS: found header Content-length or Transfer-encoding");
                    _parsingPhase = PARSING_BODY;
                    continue;
                } else {
                    DEBUG_LOG(
                        "PARSING_HEADERS: didn't find header Content-length or Transfer-encoding, parsing complete");
                    _parsingPhase = PARSING_COMPLETE;
                }
            }
            if (_parsingPhase == PARSING_BODY) {
                DEBUG_LOG("_bodyBuffer.size(): " + toString(_bodyBuffer.size()));
                parseBody(req, maxBodySize);
                DEBUG_LOG("PARSING_BODY");
                _parsingPhase = PARSING_COMPLETE;
            }
            if (_parsingPhase == PARSING_COMPLETE) {
                DEBUG_LOG("PARSING_COMPLETE");
                if (_accumulator.empty()) {
                    DEBUG_LOG("accumulator empty");
                } else {
                    DEBUG_LOG("accumulator not empty: {" + _accumulator + "}");
                }
                reqQueue.push(req);
                req = Request();
                this->resetParser();
                _parsingPhase = PARSING_START_LINE;
            }
        } catch (RequestParsingError& e) {
            DEBUG_LOG(e.what());
            return handleParseError(req, reqQueue);
        }
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
