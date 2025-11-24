// RequestParserStartLine.cpp

#include "Enums.hpp"
#include "Request.hpp"
#include "RequestParser.hpp"
#include "Utils.hpp"
#include "Webserv.hpp"

void RequestParser::parseStartLine(Request& req) {
    std::vector<std::string> split;

    /* split line */
    splitStartLine(split, _startLine);

    /* parse method */
    parseMethod(split[0], req);

    /* set request-target path and query-string */
    parsePathAndQueryString(split[1], req);

    /* set HTTP protocol version */
    parseProtocolVersion(split[2], req);
}

// splits the line in three. Throws if less than two spaces found
void RequestParser::splitStartLine(std::vector<std::string>& split, std::string& line) {
    size_t pos;

    for (size_t i = 0; i < 2; i++) {
        pos = line.find(' ');
        if (pos == line.npos)
            throw RequestParsingError("splitStartLine(): found less than two spaces - {" + line + "}");
        split.push_back(line.substr(0, pos));
        line = line.substr(pos + 1);
    }
    split.push_back(line.substr(0));
    return;
}

void RequestParser::parseMethod(std::string& split0, Request& req) {
    if (split0.empty())
        throw RequestParsingError("parseMethod(): method field is empty");

    req.setMethod(split0);

    if (req.getMethod() != GET && req.getMethod() != POST && req.getMethod() != DELETE) {
        req.setStatusCode(NOT_IMPLEMENTED);
        throw RequestParsingError("parseMethod(): method not implemented");
    }
}

void RequestParser::parsePathAndQueryString(std::string& split1, Request& req) {
    size_t queryPos;

    if (split1.empty() || split1.find_first_of(" \t\n\r\f\v") != std::string::npos)
        throw RequestParsingError("parsePathAndQueryString(): ");
    queryPos = split1.find("?");
    if (queryPos != std::string::npos) {
        req.setQueryString(split1.substr(queryPos + 1));
        split1 = split1.substr(0, queryPos);
    }
    req.setPath(split1);

    if (!startsWith(req.getPath(), "http://") && !startsWith(req.getPath(), "/"))
        throw RequestParsingError(
            "parsePathAndQueryString() - invalid request-target: does not start with \"http://\" or \"/\"");
}

void RequestParser::parseProtocolVersion(std::string& split2, Request& req) {
    if (split2.empty())
        throw RequestParsingError("parseProtocolVersion(): ProtocolVersion field is empty");

    req.setProtocolVersion(split2);

    if (split2 != "HTTP/1.0" && split2 != "HTTP/1.1" && split2 != "HTTP/0.9" && split2 != "HTTP/2" &&
        split2 != "HTTP/3") {
        DEBUG_LOG("RequestParser: split2: " + split2);
        throw RequestParsingError("parseProtocolVersion(): HTTP version \"" + split2 + "\" not recognized");
    }

    if (split2 != "HTTP/1.1") {
        req.setStatusCode(HTTP_VERSION_NOT_SUPPORTED);
        throw RequestParsingError("parseStartLine(): HTTP version \"" + split2 + "\" not supported");
    }
}
