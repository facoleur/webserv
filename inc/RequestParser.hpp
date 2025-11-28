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

    void resetParser(void);

    // Main function
    void feed(char*, std::queue<Request>&, size_t);

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
    void parseStartLine(Request&);
    void parseHeaders(Request&, size_t);
    void parseHeader(std::string&, Request&, size_t);
    void parseBody(Request&, size_t);

    // sub-parsing functions
    void parsePathAndQueryString(std::string&, Request&);
    void parseMethod(std::string&, Request&);
    void parseProtocolVersion(std::string&, Request&);
    void handleHeaderContentLength(Request&, const headersMap&, size_t);
    void extractStartLineFromFirstSection(void);

    // Validation
    void validateHeaders(Request&, size_t);
    bool isValidBody(Request&) const; // used ?

    // Helpers
    void                                splitStartLine(std::vector<std::string>&, std::string&);
    std::pair<std::string, std::string> checkHeaderSyntax(std::string&, Request&);
    void                                fillHeadersMap(std::pair<std::string, std::string> const&, Request&);
    bool                                isCaseInsensitiveHeader(std::string&);
    void                                initHeaderStringToEnumMap(void);

    // Error handling
    void handleParseError(Request&, std::queue<Request>&, const char*);

    // attributes
    ParserState                           _parserState;
    ParsingPhase                          _parsingPhase;
    size_t                                _contentLength;
    std::string                           _accumulator;
    std::string                           _firstSection; // request-line + headers
    std::string                           _startLine;
    std::string                           _headersBuffer;
    std::string                           _bodyBuffer;
    std::map<std::string, requestHeaders> _headerStringToEnum;
};
