// RequestParser.cpp

#include "RequestParser.hpp"
#include "Server.hpp"

RequestParser::RequestParser(void)
    : _parserState(REQ_PARSE_START), _parsingPhase(PARSING_REQUEST_LINE), _statusCode(NO_STATUS), _contentLength(0),
      _accumulator(), _firstSection(), _requestLine(), _headers(), _body() {
}

RequestParser::~RequestParser(void) {
}

enum ParserState RequestParser::getState(void) {
    return _parserState;
}

// splits the line in three. Throws if less than two spaces found
void RequestParser::splitRequestLine(std::vector<std::string>& split, std::string& line) {
    size_t pos;

    for (size_t i = 0; i < 2; i++) {
        pos = line.find(' ');
        if (pos == line.npos)
            throw RequestParsingError();
        split.push_back(line.substr(0, pos));
        line = line.substr(pos + 1);
    }
    split.push_back(line.substr(0));
    return;
}

void RequestParser::parseRequestLine(Request& req) {
    std::vector<std::string> split;
    size_t                   queryPos;

    /* split line */
    splitRequestLine(split, _requestLine);
	
	/* set method */
    if (split[0].empty())
        throw RequestParsingError();
	req.setMethod(split[0]);

    /* set request-target path and query-string */
    if (split[1].empty())
        throw RequestParsingError();
    if (split[1][0] != '/')
        throw RequestParsingError();
    if (split[1].find_first_of(" \t\n\r\f\v") != std::string::npos)
        throw RequestParsingError();
    queryPos = split[1].find("?");
    if (queryPos != std::string::npos) {
        req.setQueryString(split[1].substr(queryPos + 1));
        split[1] = split[1].substr(0, queryPos);
    }
    req.setPath(split[1]);

    /* set HTTP version */
    if (split[2] != "HTTP/1.0" && split[2] != "HTTP/1.1")
        throw RequestParsingError();
    req.setProtocolVersion(split[2]);
}

void RequestParser::handleParseError(Request& req, std::queue<Request>& reqQueue) {
	DEBUG_LOG("Parse error");
    req.setValidity(INVALID_REQUEST);
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
            case PARSING_REQUEST_LINE:
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
                    if (_firstSection.size() >= READ_BUF_SIZE)
                        return handleParseError(req, reqQueue);
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
            if (_parsingPhase == PARSING_REQUEST_LINE) // internal
            {
                pos = _firstSection.find(CRLF);
                if (pos != std::string::npos) { // request-line + headers
                    _requestLine  = _firstSection.substr(0, pos);
                    _firstSection = _firstSection.substr(pos + 2);
                    parseRequestLine(req);
                    _parsingPhase = PARSING_HEADERS;
                } else { // pure request-line, no headers
                    _requestLine = _firstSection;
                    _firstSection.clear();
                    parseRequestLine(req);
                    _parsingPhase = PARSING_COMPLETE;
                }
            }
            if (_parsingPhase == PARSING_HEADERS) {
                // 	parseHeaders();
				if (true) // has header content-length
					_parsingPhase = PARSING_BODY;
				else
					_parsingPhase = PARSING_COMPLETE;
            }
            if (_parsingPhase == PARSING_BODY) {
                // 	parseBody();
				_parsingPhase = PARSING_COMPLETE;
            }
            if (_parsingPhase == PARSING_COMPLETE) {
                req.validateRequest();
                reqQueue.push(req);
                if (req.getValidity() == INVALID_REQUEST)
                    return;
				req = Request();
				_parsingPhase = PARSING_REQUEST_LINE;
            }
        } catch (RequestParsingError& e) {
            return handleParseError(req, reqQueue);
        }
    }
    _parserState = REQ_PARSE_COMPLETE;
}

// HOW TO USE IT IN SERVER LOOP
// Read 1000 bytes:

// request:
// GET /trucvalide HTTP/1.1

// response:
//

// GET/ HTTP/1.0 => poubelle
// Host: 127.0.0.1 => poubelle
// GET /root HTTP/1.1
// Host: blabla

/* IN SERVER.CPP LOOP:

    while (1)
    {
        std::map<int, RequestParser> request_parsers;
        Request *req = new Request();

        // ...

        poll(pfds);
        ssize_t ret = read(sockFd, buf, READ_BUF_SIZE);
        if (read <= 0)
            return handle_read_error...
                request_parsers[sockFd].feed(buf, req); // call the parser on the buffer to fill the Request
                enum ParserState ps = request_parsers[sockFd].getState());
                if (ps == REQ_PARSE_PARTIAL)
                        continue;
                handle_requests(req_queue);
                if (REQ_PARSE_ERROR) // problem => if Request parse ok but Request is semantically invalid, then ... ?
solution below: close(sockFd);

                                // SOLUTION:
                // bool allRequestsValid = handle_requests(req_queue);
                // if (!allRequestsValid) // => this means REQ_PARSE_ERROR isn't actually that necessary. Only
REQ_PARSE_PARTIAL
                //        close(sockFd);
        }

handle_requests(queue requests)
{
        if (fd == POLLOUT)
                read(fichier, buf, 8196);
                write(clientFd, buf, 8196);
}
*/