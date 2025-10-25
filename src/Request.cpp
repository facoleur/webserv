// Request.cpp

#include "Request.hpp"

Request::Request(void) : _validity(INVALID_REQUEST) {
}

Request::~Request(void) {
}

// Request::setValidity()

// in class Request OR in handle_requests
// if (INVALID_REQUEST)
// {
	// if (req.getmethod() == method not found)
	// 		setStatusCode(METHOD_NOT_FOUND)
	// if (blabla)
	// 		setStatusCode(BLA_BLA)
	// else
	// 		setStatusCode(BAD_REQUEST)
// }