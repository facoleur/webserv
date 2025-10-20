// MockRequestParser.cpp

#include "MockRequestParser.hpp"

MockRequest* MockRequestParser::parse(int sockFd) {
    char    _buf[REQ_BUF_SIZE + 1];
    ssize_t ret = recv(sockFd, _buf, REQ_BUF_SIZE, 0);

    if (ret <= 0) {
        std::cerr << "recv() failed or client disconnected\n";
        return NULL;
    }

    _buf[ret] = '\0';
    std::cout << "Received raw request:\n" << _buf << std::endl;

    MockRequest* req = new MockRequest();

    return req;
}
