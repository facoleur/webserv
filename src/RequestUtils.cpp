#include "Request.hpp"
#include <ostream>

std::ostream& operator<<(std::ostream& os, requestMethod method) {
    switch (method) {
        case GET:
            os << "GET";
            break;
        case POST:
            os << "POST";
            break;
        case DELETE:
            os << "DELETE";
            break;
        default:
            os << "UNKNOWN";
            break;
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, enum requestValidity reqVal) {
    switch (reqVal) {
        case VALID_REQUEST:
            os << "VALID_REQUEST";
            break;
        case INVALID_REQUEST:
            os << "INVALID_REQUEST";
            break;
        default:
            os << "ERROR: UNKNOWN_VALIDITY";
            break;
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, enum statusCode stat) {
    switch (stat) {
        case NO_STATUS:
            os << "NO_STATUS";
            break;
        case OK:
            os << "OK";
            break;
        case ACCEPTED:
            os << "ACCEPTED";
            break;
        case NO_CONTENT:
            os << "NO_CONTENT";
            break;
        case BAD_REQUEST:
            os << "BAD_REQUEST";
            break;
        case FORBIDDEN:
            os << "FORBIDDEN";
            break;
        case NOT_FOUND:
            os << "NOT_FOUND";
            break;
        case NOT_ALLOWED:
            os << "NOT_ALLOWED";
            break;
        case INTERNAL_SERVER_ERROR:
            os << "INTERNAL_SERVER_ERROR";
            break;
        case NOT_IMPLEMENTED:
            os << "NOT_IMPLEMENTED";
            break;
        case HTTP_VERSION_NOT_SUPPORTED:
            os << "HTTP_VERSION_NOT_SUPPORTED";
            break;
        default:
            os << "NO_STATUS";
            break;
    }
    return os;
}

void Request::printHeaders(std::ostream& os) {

    for (std::map<enum requestHeaders, std::string>::iterator it = _headers.begin(); it != _headers.end(); it++) {
        os << (*it).first << ": " << (*it).second << std::endl;
    }
}

std::ostream& operator<<(std::ostream& os, Request& req) {
    os << "[REQUEST]" << std::endl;
    os << "--------------------------------" << std::endl;
    os << "Method:           " << req._method << std::endl;
    os << "Path:             " << req._path << std::endl;
    os << "Query String:     " << req._queryString << std::endl;
    os << "Headers:          ";
    req.printHeaders(os);
    os << std::endl;
    os << "Body:             " << req._body << std::endl;
    os << "Protocol version: " << req._protocolVersion << std::endl;
    os << "Status code:      " << req._statusCode << std::endl;
    os << "Validity:         " << req._validity << std::endl;
    os << "--------------------------------" << std::endl;
    return os;
}
