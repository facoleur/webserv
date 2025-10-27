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
    void feed(char* buf, std::queue<Request> &req_queue);

    // getters
    enum ParserState getState(void);

    struct RequestParsingError : std::exception { };

  private:
    void    parseRequestLine(bool);
    void    parseHeaders(void);
  
    void    parseMethod(std::string const& token);
    void    parseHeader(std::string&);
    void    parseBody(int sockFd, size_t contentLength);

    void    handleParseError(Request &, std::queue<Request> &);
    void    handleParsePartial(size_t);

    // attributes

    enum ParserState  _parserState;
    enum ParsingPhase _parsingPhase;
    enum StatusCode   _statusCode;
    size_t            _contentLength;
    std::string       _accumulator;
    std::string       _firstSection; // request-line + headers
    std::string       _requestLine;
    std::string       _headers;
};
