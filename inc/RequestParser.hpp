// Request Parser.hpp

#pragma once

#include <cstddef>
#include <queue>

#include "Enums.hpp"

#define READ_BUF_SIZE 8000
#define MIN_REQ_SIZE 19            // shortest possible request without ending CRLFCRLF: "GET / HTTP/2\r\nHost:"
#define MIN_HEADER_SIZE 3          // header name + field min length ("H:I")
#define MAX_HEADER_SIZE 4000       // header name + field max length
#define MAX_CHUNK_SIZE 268435456   // 16^7 => 7 hex digits
#define MAX_CHUNK_SIZE_LINE_SIZE 7 // 7 hex digits
#define CRLF std::string("\r\n")
#define READ_MORE 1
#define CONTENT_LENGTH_OK 2
#define CHUNK_SIZE_OK 3
#define PARSE_MORE_CHUNKS 4
#define CHUNK_FINISHED 5
#define FIRST_SECTION_OK 6

class Request;

/* parses an HTTP request, (in)validating its syntax, and storing the result in
a Request object */
class RequestParser {

  public:
    // Constructors
    RequestParser(void);
    ~RequestParser(void);

    // public functions
    void feed(char*, std::queue<Request>&, size_t);
    void resetParser(void);

    // getters
    enum ParserState getState(void) const;
    void             setState(enum ParserState);

    // parser error
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
    void parseHeaders(Request&);
    void parseHeader(std::string&, Request&);
    void parseFullBody(Request&);

    // sub-parsing functions
    int  extractFirstSection(void);
    void extractStartLineFromFirstSection(void);
    int  extractFullBody(void);
    int  extractChunkSize(size_t);
    int  extractChunkData(void);
    void parsePathAndQueryString(std::string&, Request&);
    void parseMethod(std::string&, Request&);
    void parseProtocolVersion(std::string&, Request&);
    void handleHeaderContentLength(Request&, const headersMap&, size_t);

    // Validation
    void validateHeaders(Request&);
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
    std::string                           _accumulator;
    std::string                           _firstSection; // start-line + headers
    std::string                           _startLine;
    std::string                           _headersBuffer;
    std::map<std::string, requestHeaders> _headerStringToEnum;
    int                                   _contentLength;
    int                                   _chunkSize;
    std::string                           _bodyBuffer;

    size_t _tmpMaxBodySize;
};
