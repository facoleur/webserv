// Request.hpp

#pragma once

#include <string>
#include <vector>

enum requestMethod
{
    GET,
    POST,
    DELETE,
    UNKNOWN
};

enum requestValidity
{
	VALID_REQUEST,
	INVALID_REQUEST
};

// stores and validates (semantically) an HTTP request
class Request
{
	private:
			// Attributes
		enum requestMethod						_method; 
		std::string								_path;
		std::string								_queryString;
		int										_protocolVersion;
		std::vector<std::string, std::string>	_headers; // lferro: use map and append during parsing if duplicate
		std::string								_body;
		enum requestValidity					_validity;

	public:
			// Constructors
		Request(void);
		~Request(void);

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

			
				// setters
		void									setMethod(enum requestMethod); 
		void									setPath(std::string);
		void									setQueryString(std::string);
		void									setProtocolVersion(int);
		void									setHeaders(
										std::vector<std::string, std::string>);
		void									setBody(std::string);
		void									setValidity(enum requestValidity);
};
