// Request Parser.hpp

#pragma once

#include "Request.hpp"
#include <queue>
#include <sstream>

#define READ_BUF_SIZE 4096
#define CRLF std::string("\r\n")

enum ParserState { REQ_PARSE_PARTIAL, REQ_PARSE_COMPLETE, REQ_PARSE_ERROR };

enum ParsingPhase { PARSING_REQUEST_LINE, PARSING_HEADERS, PARSING_BODY, PARSING_COMPLETE };

/* parses an HTTP request, (in)validating its syntax, and storing the result in
a Request object */
class RequestParser {

  public:
    RequestParser();

    void feed(char* buf, std::queue<Request>& req);

    // getters
    enum ParserState getState(void);

  private:
    void* parseRequestLine(std::string& line);
    void* parseMethod(std::string const& token);
    void* parseHeaders(int sockFd);
    void* parseHeader(std::string& line);
    void* parseBody(int sockFd, size_t contentLength);

    // handlers
    void skipBytes(void); // advance by Content-Length bytes

    // attributes
    std::string       _accumulator;
    size_t            _bufferPos;
    enum ParserState  _parserState;
    enum ParsingPhase _parsingPhase;
};

// HOW TO USE IT
// Read 1000 bytes:

// request:
// GET /trucvalide HTTP/1.1

// response:
//

// GET/ HTTP/1.0 => poubelle
// Host: 127.0.0.1 => poubelle
// GET /root HTTP/1.1
// Host: blabla

/* IN SERVER.CPP LOOP:

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
                                        while (request_parsers[sockFd].getState() == REQ_PARSE_COMPLETE)
                                        {
                                                handle_request(req);
                                                request_parsers[sockFd].feed(buf, req);
                                        }
                                        if (request_parsers[sockFd].getState() == REQ_PARSE_ERROR)
                                                return REQ_PARSE_ERR;
                                        else
                                                continue;
                case REQ_PARSE_ERROR:
                    req_par.clear(req); // clear the parser and the Request
                    return REQ_PARSE_ERR;


        }
    }

handle_requests(queue requests)
{
        if (fd == POLLOUT)
                read(fichier, buf, 8196);
                write(clientFd, buf, 8196);
}


*/
