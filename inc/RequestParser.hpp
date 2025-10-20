// RequestParser.hpp

#pragma once

#include <sstream>
#include <string>
// #include <sys/types.h>
#include "Request.hpp"
#include <sys/socket.h>

#define READ_BUF_SIZE 4096

enum ParserState { REQ_PARSE_PARTIAL, REQ_PARSE_COMPLETE, REQ_PARSE_ERROR };

/* parses an HTTP request, (in)validating its syntax, and storing the result in
a Request object */
class RequestParser {

  public:
    void feed(char* buf, Request* req);

  private:
    void* parseRequestLine(std::string const& line);
    void* parseMethod(std::string const& token);
    void* parseHeaders(int sockFd);
    void* parseHeader(std::string const& line);
    void* parseBody(int sockFd, size_t contentLength);

    // handlers
    size_t findCRLF(void);
    void   skipBytes(void); // advance by Content-Length bytes
    void   setState(enum ParserState);

    // getters
    enum ParserState getState(void);

    // attributes
    std::string      _accumulator;
    size_t           _bufferPos;
    enum ParserState _state;
};

// Read 1000 bytes:

// request:
// GET /trucvalide HTTP/1.1

// response:
//

// GET/ HTTP/1.0 => poubelle
// Host: 127.0.0.1 => poubelle
// GET /root HTTP/1.1
// Host: blabla

// ssize_t recv(int sockfd, void *buf, size_t len, int flags);

/* In Server.cpp loop:




    while (1)
    {
        std::map<int, RequestParser> request_parsers;
        Request *req = new Request();

        // ...

        poll(pfds);
        ssize_t ret = read(sockFd, buf, READ_BUF_SIZE);
        if (read <= 0)
            return handle_cases...
        else
        {
            request_parsers[sockFd].feed(buf, req); // call the parser on the buffer to fill the
   Request

            switch (request_parsers[sockFd].getState()) { // check if a Request could be parsed
   from the buffer,
                                          // or if more bytes need to be read
                case REQ_PARSE_PARTIAL:
                    continue;
                case REQ_PARSE_COMPLETE:
                    handle_request(req);
                case REQ_PARSE_ERROR:
                    req_par.clear(req); // clear the parser and the Request
                    return REQ_PARSE_ERR;
        }
    }

*/

void RequestParser::feed(char* buf, Request* req) {
    std::string line;

    if (REQ_PARSE_PARTIAL)
        _accumulator += buf;
    size_t pos = findCRLF();

    if (pos != _accumulator.npos)
        line = _accumulator.extractUntilPos(pos);
    else {
        setState(REQ_PARSE_PARTIAL);
        return;
    }
}

size_t RequestParser::findCRLF(void) {
}
