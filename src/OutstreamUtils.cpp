// OutstreamUtils.cpp

#include <ostream>
#include <poll.h>

#include "Enums.hpp"

std::ostream& operator<<(std::ostream& os, const struct pollfd pfd) {
    os << "fd: " << pfd.fd << std::endl;
    os << "events: " << pfd.events << std::endl;
    os << "revents: " << pfd.revents << std::endl;
    return os;
}

std::ostream& operator<<(std::ostream& os, requestHeaders headerName) {
    switch (headerName) {
        case HOST:
            os << "HOST";
            break;
        case CONTENT_LENGTH:
            os << "CONTENT_LENGTH";
            break;
        case LOCATION:
            os << "LOCATION";
            break;
        case TRANSFER_ENCODING:
            os << "TRANSFER_ENCODING";
            break;
        case CONTENT_TYPE:
            os << "CONTENT_TYPE";
            break;
        case CONNECTION:
            os << "CONNECTION";
            break;
        case ACCEPT:
            os << "ACCEPT";
            break;
        default:
            os << "ERROR: UNKNOWN_HEADER";
            break;
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, const headersMap& headersMap) {
    os << std::endl;
    for (headersMap::const_iterator it = headersMap.begin(); it != headersMap.end(); it++) {
        os << "\t" << (*it).first << ": " << (*it).second << std::endl;
    }
    return os;
}

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
            os << "ERROR: UNKNOWN_METHOD";
            break;
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, requestValidity reqVal) {
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

std::ostream& operator<<(std::ostream& os, statusCode stat) {
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
            os << "ERROR: UNKNOWN_STATUS";
            break;
    }
    return os;
}
