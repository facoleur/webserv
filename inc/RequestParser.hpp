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

RequestParser::RequestParser(void) : _parsingPhase(PARSING_REQUEST_LINE),
    _parserState(REQ_PARSE_START), _accumulator(), _contentLength(0), _statusCode(NO_STATUS) {
}

void RequestParser::parseRequestLine(std::string& line)
{
    /* split line */
    /* checks:
        - only three elements 
        - not more than two spaces
    */
}

void RequestParser::feed(char* buf, std::queue<Request> req_queue) 
{
    std::string line;
    size_t      pos;
    Request     req;

    /* add the buffer to the accumulator */
    _accumulator += buf;
    
    // if (_parserState == REQ_PARSE_START)
    //     req_queue;

    /* Parse line by line */
    for (std::string::iterator it = _accumulator.begin(); it != _accumulator.end(); ++it)
    {
        if (_parsingPhase == PARSING_REQUEST_LINE || _parsingPhase == PARSING_HEADERS) 
        {
            pos = _accumulator.find(CRLF);
            if (pos == _accumulator.npos) 
            { // 8196 Bytes read, no CRLF found => line larger than 8194 Bytes, so error
                req.setValidity(INVALID_REQUEST);
                req.setStatusCode(BAD_REQUEST);
                _parsingPhase = REQ_PARSE_COMPLETE;
                _parserState = REQ_PARSE_ERROR;
                return;
            }
            line         = _accumulator.substr(0, pos);
            _accumulator = _accumulator.substr(pos + 2); // + 2 to skip CRLF
        }
        else if (_parsingPhase == PARSING_BODY) 
        {
			// check header content-length _contentLength
            _parserState = REQ_PARSE_PARTIAL;

            return;
        }

        /* Parse the line */
        try
        {
            switch (_parsingPhase)
            {
                case PARSING_REQUEST_LINE:
                    parseRequestLine(line);
                    _parsingPhase = PARSING_HEADERS;
                    break;
                case PARSING_COMPLETE:
                    break;
            }
            // ... how to handle adding headers ?
        }
        catch (std::exception& e)
        {
            std::cout << e.what() << std::endl;
            _parserState = REQ_PARSE_ERROR;
            break;
        }

        if (_parsingPhase == PARSING_COMPLETE)
            break;
    }
    // testFeed();
}


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
                    close(sockFd);
                    
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