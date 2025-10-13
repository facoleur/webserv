// Request.hpp

#pragma once

enum requestMethod
{
    GET,
    POST,
    DELETE,
    UNKNOWN
};

enum requestValidity
{
	UNEVALUATED_REQUEST,
	VALID_REQUEST,
	INVALID_REQUEST
};

// stores and validates an HTTP request
class Request
{
	// Attributes
	enum requestMethod						_method; 
	std::string								_path;
	std::string								_queryString;
	int										_protocolVersion;
	std::vector<std::string, std::string>	_headers; // lferro: use map and append during parsing if duplicate
	std::string								_body;
	enum requestValidity					_validity;

	// Functions
		// presence checks
	bool									hasHeader(std::string const &); // whether a specific header is present
	bool									hasBody(void);

		// validity checks => semantic validation
	bool									isMethodValid(void);
	bool									isPathValid(void);
	bool									isQueryStringValid(void);
	bool									isProtocolVersionValid(void);
	bool									isHeadersValid(void);
	bool									isBodyValid(void);
	bool									isRequestValid(void);
	
		// getters
	enum requestMethod						getMethod(void); 
	std::string								&getPath(void);
	std::string								&getQueryString(void);
	int										getProtocolVersion(void);
	std::vector<std::string, std::string>	&getHeaders(void);
	std::string								&getBody(void);
	enum requestValidity					getValidity(void);

};

// parses an HTTP request, (in)validating its syntax, and storing the result in a Request object
class RequestParser {

	parseRequestLine(std::string const &line);
	parseMethod(std::string const &token);
	parseHeaders(int socketFd);
	parseHeader(std::string const &line);
	parseBody(int socketFd, size_t contentLength);

		// handlers
			skipBytes(void); // advance by Content-Length bytes
};
