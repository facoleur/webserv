// RequestParser.cpp

#include "RequestParser.hpp"
#include "Enums.hpp"
#include "Request.hpp"
#include "Utils.hpp"
#include "Webserv.hpp"

RequestParser::RequestParser(void)
    : _parserState(REQ_PARSE_START), _parsingPhase(PARSING_START_LINE), _statusCode(NO_STATUS), _contentLength(0),
      _accumulator(), _firstSection(), _startLine(), _headers(), _body() {
}

RequestParser::~RequestParser(void) {
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

void RequestParser::feed(char* buf, std::queue<Request>& reqQueue) {
    size_t  pos;
    Request req;

    _accumulator += buf;
    while (!_accumulator.empty()) {
        /* 1. extract the content */
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
                        DEBUG_LOG("size: " + toString(_firstSection.size()) + " is below MIN_REQ_SIZE" +
                                  "\n_firstSection:" + _firstSection);
                        return handleParseError(req, reqQueue);
                    }
                    _accumulator = _accumulator.substr(pos + 4);
                }
                break;
            case PARSING_BODY:
                // ...
                break;
            case PARSING_COMPLETE:
                // ??
                break;
            default:
                break;
        }

        /* 2. parse the extracted content */
        try {
            if (_parsingPhase == PARSING_START_LINE) // internal
            {
                pos = _firstSection.find(CRLF);
                if (pos != std::string::npos) { // _firstSection has start-line + headers
                    _startLine    = _firstSection.substr(0, pos);
                    _firstSection = _firstSection.substr(pos + 2);
                    parseStartLine(req);
                    _parsingPhase = PARSING_HEADERS;
                } else { // _firstSection is a pure start-line with no headers
                    DEBUG_LOG("exiting at PARSING_START_LINE: no headers found");
                    return handleParseError(req, reqQueue);
                }
            }
            if (_parsingPhase == PARSING_HEADERS) {
                _headers = _firstSection;
                parseHeaders(req);
                if (req.hasHeader(CONTENT_LENGTH) || req.hasHeader(TRANSFER_ENCODING))
                    _parsingPhase = PARSING_BODY;
                else
                    _parsingPhase = PARSING_COMPLETE;
            }
            if (_parsingPhase == PARSING_BODY) {
                // 	parseBody();
                _parsingPhase = PARSING_COMPLETE;
            }
            if (_parsingPhase == PARSING_COMPLETE) {
                reqQueue.push(req);
                req           = Request();
                _parsingPhase = PARSING_START_LINE;
            }
        } catch (RequestParsingError& e) {
            // DEBUG_LOG(e.what());
            std::cerr << e.what() << std::endl; // REMOVE IN PROD
            return handleParseError(req, reqQueue);
        }
    }
    _parserState = REQ_PARSE_COMPLETE;
}
