// MockRequestParser.hpp

#pragma once

#include <iostream>
#include <string>
// #include <sys/types.h>
#include "MockRequest.hpp"
#include <sys/socket.h>

class MockRequest;

#define REQ_BUF_SIZE 4096
/* parses an HTTP request, (in)validating its syntax, and storing the result in
a Request object */
class MockRequestParser {
  public:
    MockRequest* parse(int sockFd);

    void* parseRequestLine(std::string const& line);
    void* parseMethod(std::string const& token);
    void* parseHeaders(int sockFd);
    void* parseHeader(std::string const& line);
    void* parseBody(int sockFd, size_t contentLength);

    // handlers
    void skipBytes(void); // advance by Content-Length bytes

  private:
    char   _buf[REQ_BUF_SIZE];
    size_t _bufferPos;
};

// ssize_t recv(int sockfd, void *buf, size_t len, int flags);
