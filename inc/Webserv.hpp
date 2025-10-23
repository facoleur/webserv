// Webserv.hpp
// General includes

#pragma once

#include <iostream>
#include <string>

enum StatusCode
{
	NO_STATUS = 0,
	BAD_REQUEST = 400,
	FORBIDDEN = 403,
	NOT_FOUND = 404,
	NOT_ALLOWED = 405, // checked if relevant => yes (also in Kaydoo's)
	INTERNAL_SERVER_ERROR = 500,
	NOT_IMPLEMENTED = 501,
	HTTP_VERSION_NOT_SUPPORTED = 505
};
