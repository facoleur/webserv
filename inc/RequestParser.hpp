// Request Parser.hpp

#pragma once

#include <queue>

#include "Enums.hpp"

#define READ_BUF_SIZE 8000
#define MIN_REQ_SIZE 19      // shortest possible request without ending CRLFCRLF: "GET / HTTP/2\r\nHost:"
#define MIN_HEADER_SIZE 3    // header name + field min length ("H:I")
#define MAX_HEADER_SIZE 4000 // header name + field max length
#define CRLF std::string("\r\n")

class Request;

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
    void             setState(enum ParserState);

    struct RequestParsingError : public std::exception {
        RequestParsingError(const std::string& msg) throw() : _message(msg) {
        }
        virtual ~RequestParsingError(void) throw() {
        }
        virtual const char* what() const throw() {
            return _message.c_str();
        }

      private:
        std::string _message;
    };

  private:
    // Main parsing functions
    void parseRequestLine(Request&);
    void parseHeaders(Request&);
    void parseHeader(std::string&, Request&);
    void parseBody(size_t contentLength);

    // Helpers
    void                                splitRequestLine(std::vector<std::string>&, std::string&);
    std::pair<std::string, std::string> checkHeaderSyntax(std::string&, Request&);
    void                                fillHeadersMap(std::pair<std::string, std::string> const&, Request&);
    unsigned char                       toLowerChar(unsigned char c);
    void                                trimWhitespace(std::string&);
    bool                                isCaseInsensitiveHeader(std::string&);
    void                                initHeaderStringToEnumMap(void);

    // Error handling
    void handleParseError(Request&, std::queue<Request>&);
    void handleParsePartial(size_t);

    // attributes
    ParserState                           _parserState;
    ParsingPhase                          _parsingPhase;
    statusCode                            _statusCode;
    size_t                                _contentLength;
    std::string                           _accumulator;
    std::string                           _firstSection; // request-line + headers
    std::string                           _requestLine;
    std::string                           _headers;
    std::string                           _body;
    std::map<std::string, requestHeaders> _headerStringToEnum;
};
