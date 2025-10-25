// RequestParser.hpp

#pragma once

#include "Request.hpp"
#include <sstream>
#include <queue>

#define READ_BUF_SIZE 8192
#define MAX_LINE_SIZE 8016
#define CRLF std::string("\r\n")

enum ParserState { REQ_PARSE_START, REQ_PARSE_PARTIAL, REQ_PARSE_COMPLETE, REQ_PARSE_ERROR };

enum ParsingPhase { PARSING_REQUEST_LINE, PARSING_HEADERS, PARSING_BODY, PARSING_COMPLETE };

/* parses an HTTP request, (in)validating its syntax, and storing the result in
a Request object */
class RequestParser {

  public:
    void feed(char* buf, std::queue<Request> req_queue);

    // getters
    enum ParserState getState(void);

  private:
    void    parseRequestLine(std::string& line);
    void* parseMethod(std::string const& token);
    void* parseHeaders(int sockFd);
    void* parseHeader(std::string& line);
    void* parseBody(int sockFd, size_t contentLength);

    // attributes
    std::string       _accumulator;
    size_t            _contentLength;
    enum ParserState  _parserState;
    enum ParsingPhase _parsingPhase;
    enum StatusCode   _statusCode;
};
