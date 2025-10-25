// RequestParser.cpp

#include "RequestParser.hpp"

void RequestParser::feed(char* buf, std::queue<Request>& req_queue) {
    std::string line;
    size_t      pos;
    Request     req;

    req_queue.push(req);

    // char* buf(8000 bytes) = "<request-line><header1><header2>....<header_n><header_n+1:"

    /* add the buffer to the accumulator */
    // _accumulator += buf;

    // if end headers (0crlfcrlf)
    // parse_req()
    // req.push()
    // continue;

    // if end feed() && (!0crlfcrlf || body_read < content-length))
    // set_state(PARTIAL)

    // return

    // Request mockRequest;
    // mockRequest.mockRequest();
    // req.push(mockRequest);

    // while (_accumulator) {
    //     /* Get a new line */
    //     if (_parsingPhase == PARSING_REQUEST_LINE || _parsingPhase == PARSING_HEADERS) {
    //         pos = _accumulator.find(CRLF);
    //         if (pos != _accumulator.npos && _accumulator.size() > 8000) {
    //             line         = _accumulator.substr(0, pos);
    //             _accumulator = _accumulator.substr(pos + 2); // + 2 to skip CRLF
    //         } else if {
    //             _parserState = REQ_PARSE_PARTIAL;
    //             return;
    //         } else if { // if body
    // ....
    //             _parserState = REQ_PARSE_PARTIAL;
    //             return;
    //         }
    //         /* Parse the line */
    //         try {
    //             switch (_parsingPhase) {
    //                 case PARSING_REQUEST_LINE:
    //                     parseRequestLine(line);
    //                     _parsingPhase = PARSING_HEADERS;
    //                     break;
    //             }
    //             // ... how to handle adding headers ?
    //         } catch (std::exception& e) {
    //             std::cout << e.what() << std::endl;
    //             _parserState = REQ_PARSE_ERROR;
    //         }
    //         _accumulator.eraseLine(line); // to implement => USE STRINGSTREAMS !
    //         if (_parsingPhase == PARSING_COMPLETE)
    //             break;
    //     }
    // req_queue.push(req);
}

RequestParser::RequestParser() : _parsingPhase(PARSING_REQUEST_LINE) {
}

void* RequestParser::parseRequestLine(std::string& line) {
}

void* RequestParser::parseMethod(std::string const& token) {
}

void* RequestParser::parseHeaders(int sockFd) {
}

void* RequestParser::parseHeader(std::string& line) {
}

void* RequestParser::parseBody(int sockFd, size_t contentLength) {
}

enum ParserState RequestParser::getState(void) {
    return REQ_PARSE_COMPLETE;
}
