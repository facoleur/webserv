// RequestParser.cpp

#include "../inc/RequestParser.hpp"

RequestParser::RequestParser(void)
    : _parserState(REQ_PARSE_START), _parsingPhase(PARSING_REQUEST_LINE), _statusCode(NO_STATUS), _contentLength(0),
      _accumulator(), _firstSection(), _requestLine(), _headers(), _body() {
}

RequestParser::~RequestParser(void) {
}

enum ParserState RequestParser::getState(void) {
	return _parserState;
}

// splits the line in three. Throws if only two spaces found
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
    size_t                   queryPos;
    std::vector<std::string> split;

    /* split line */
    splitRequestLine(split, _requestLine);

    /* set method */
    if (split[0].empty())
        throw RequestParsingError();
    else
        req.setMethod(split[0]);

    /* set request target path and query-string */
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
    if (split[2] == "HTTP/1.0" || split[2] == "HTTP/1.1")
        req.setProtocolVersion(split[2]);
    else
        throw RequestParsingError();
}

void RequestParser::handleParseError(Request& req, std::queue<Request>& reqQueue) {
    req.setValidity(INVALID_REQUEST);
    reqQueue.push(req);
    _parserState = REQ_PARSE_ERROR;
}

void RequestParser::feed(char* buf, std::queue<Request>& reqQueue) {
    size_t  pos;
    Request req;

    _accumulator += buf;

    while (!_accumulator.empty()) {
        switch (_parsingPhase) {
            case PARSING_REQUEST_LINE:
            case PARSING_HEADERS:
                pos = _accumulator.find(CRLF + CRLF);
                if (pos == std::string::npos && _accumulator.size() >= READ_BUF_SIZE)
                    return handleParseError(req, reqQueue);
                else if (pos == std::string::npos) {
                    _firstSection += _accumulator;
                    if (_firstSection.size() >= READ_BUF_SIZE)
                        return handleParseError(req, reqQueue);
                    _accumulator.clear();
                    _parserState = REQ_PARSE_PARTIAL;
                    return;
                }
                _firstSection += _accumulator.substr(0, pos);
                if (_firstSection.size() >= READ_BUF_SIZE)
                    return handleParseError(req, reqQueue);
                _accumulator = _accumulator.substr(pos);
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

        /* Parse the line */
        try {
            switch (_parsingPhase) // internal
            {
                case PARSING_REQUEST_LINE:
                    pos = _firstSection.find(CRLF);
                    if (pos != std::string::npos) { // request-line + headers
                        _requestLine  = _firstSection.substr(0, pos);
                        _firstSection = _firstSection.substr(pos);
                        parseRequestLine(req);
                        _parsingPhase = PARSING_HEADERS;
                    } else { // pure request-line, no headers
                        _requestLine = _firstSection;
                        _firstSection.clear();
                        parseRequestLine(req);
                        _parsingPhase = PARSING_COMPLETE;
                    }
                    break;
                    // case PARSING_HEADERS:
                    // 	parseHeaders();
                    // 	break;
                    // case PARSING_BODY:
                    // 	parseBody();
                    // 	break;
                case PARSING_COMPLETE: // n.b.: this part cannot throw
                    req.validateRequest();
                    req.printRequest();
                    reqQueue.push(req);
                    if (req.getValidity() == INVALID_REQUEST)
                        return;
                    break;
                default:
                    break;
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
                if (ps == REQ_PARSE_ERROR)
                        close(sockFd);
        }

handle_requests(queue requests)
{
        if (fd == POLLOUT)
                read(fichier, buf, 8196);
                write(clientFd, buf, 8196);
}
*/