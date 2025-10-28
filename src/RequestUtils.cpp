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

std::ostream& operator<<(std::ostream& os, Request& req) {
    os << "[REQUEST]" << std::endl;
    os << "Method:       " << req._method << std::endl;
    os << "Path:         " << req._path << std::endl;
    os << "Query String: " << req._queryString << std::endl;
    os << "Body:         " << req._body << std::endl;

    return os;
}
