// RequestParser.hpp

#pragma once

#include <string>
	// #include <sys/types.h>
	#include <sys/socket.h>
#include "Request.hpp"

#define REQ_BUF_SIZE 4096
/* parses an HTTP request, (in)validating its syntax, and storing the result in 
a Request object */
class RequestParser {
	Request	*parse(int sockFd);

	void	*parseRequestLine(std::string const &line);
	void	*parseMethod(std::string const &token);
	void	*parseHeaders(int sockFd);
	void	*parseHeader(std::string const &line);
	void	*parseBody(int sockFd, size_t contentLength);

		// handlers
	void	skipBytes(void); // advance by Content-Length bytes

	private:
		char		_buf[REQ_BUF_SIZE];
		size_t		_bufferPos;
		

};

// ssize_t recv(int sockfd, void *buf, size_t len, int flags);

Request *RequestParser::parse(int sockFd)
{
	Request *req = new Request();
	ssize_t ret = recv(sockFd, _buf, REQ_BUF_SIZE, );
	if (ret == -1)
		// handle_parse_error(REQ_PARSE_ERROR); ?
	
}
