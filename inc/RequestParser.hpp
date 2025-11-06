// Request Parser.hpp

#pragma once

#include "Request.hpp"
#include <queue>
#include <sstream>

#define READ_BUF_SIZE 8000
#define CRLF std::string("\r\n")

enum ParserState { REQ_PARSE_START, REQ_PARSE_PARTIAL, REQ_PARSE_COMPLETE, REQ_PARSE_ERROR };

enum ParsingPhase { PARSING_REQUEST_LINE, PARSING_HEADERS, PARSING_BODY, PARSING_COMPLETE };

/* parses an HTTP request, (in)validating its syntax, and storing the result in
a Request object */
class RequestParser {

  public:
    // Constructors
    RequestParser(void);
    ~RequestParser(void);

    // Main function
    void feed(char*, std::queue<Request>&);

    // getters
    enum ParserState getState(void);

    struct RequestParsingError : std::exception {};

  private:
    // Main parsing functions
    void parseRequestLine(Request&);
    void parseHeaders(void);
    void parseHeader(std::string&);
    void parseBody(int sockFd, size_t contentLength);

    // Helpers
    void splitRequestLine(std::vector<std::string>&, std::string& line);

    // Error handling
    void handleParseError(Request&, std::queue<Request>&);
    void handleParsePartial(size_t);

    // attributes
    enum ParserState  _parserState;
    enum ParsingPhase _parsingPhase;
    enum statusCode   _statusCode;
    size_t            _contentLength;
    std::string       _accumulator;
    std::string       _firstSection; // request-line + headers
    std::string       _requestLine;
    std::string       _headers;
    std::string       _body;
};
