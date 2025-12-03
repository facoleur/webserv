// RequestParserHeaders.cpp

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <sstream>

#include "Enums.hpp"
#include "Request.hpp"
#include "RequestParser.hpp"
#include "Server.hpp"
#include "Utils.hpp"

void RequestParser::parseHeaders(Request& req) {
    size_t      pos;
    std::string header;

    pos = _headersBuffer.find(CRLF);
    while (pos != std::string::npos) {
        header         = _headersBuffer.substr(0, pos);
        _headersBuffer = _headersBuffer.substr(pos + 2);
        parseHeader(header, req);
        pos = _headersBuffer.find(CRLF);
    }
    if (_headersBuffer.size()) // last header
        header = _headersBuffer.substr(0, pos);
    parseHeader(header, req);
    validateHeaders(req);
    _headersBuffer.clear();
}

void RequestParser::parseHeader(std::string& header, Request& req) {
    std::pair<std::string, std::string> header_pair;

    header_pair = checkHeaderSyntax(header, req);
    fillHeadersMap(header_pair, req);
}

// splits the header line around ":" and performs syntax checks
// throws on syntax error
// returns a pair of strings: <headerName, headerField>
std::pair<std::string, std::string> RequestParser::checkHeaderSyntax(std::string& header, Request& req) {
    std::string::difference_type n;
    int                          count;
    size_t                       pos;
    std::string::iterator        it;
    std::string                  headerName;
    std::string                  headerField;

    if (header.size() < MIN_HEADER_SIZE)
        throw RequestParsingError("checkHeaderSyntax(): header < MIN_HEADER_SIZE");

    if (header.size() > MAX_HEADER_SIZE) {
        req.setStatusCode(CONTENT_TOO_LARGE);
        throw RequestParsingError("checkHeaderSyntax(): header > MAX_HEADER_SIZE");
    }

    n     = std::count(header.begin(), header.end(), ':');
    count = static_cast<int>(n);
    if (count < 1) // less than one colon
        throw RequestParsingError("checkHeaderSyntax(): header doesn't have exactly one colon (':')");

    pos = header.find(":");
    if (pos == 0 || pos == header.size() - 1) // colon is first or last character
        throw RequestParsingError("checkHeaderSyntax(): colon (':') is first or last character");

    headerName  = header.substr(0, pos);
    headerField = header.substr(pos + 1); // skip the ":"

    it = std::find_if(headerName.begin(), headerName.end(), isSpace);
    if (it != headerName.end())
        throw RequestParsingError("checkHeaderSyntax(): whitespace found before colon (':')");

    // format header name and field
    headerName  = tolower(headerName);
    headerField = trimString(headerField);
    if (isCaseInsensitiveHeader(headerName))
        headerField = tolower(headerField);

    return std::pair<std::string, std::string>(headerName, headerField);
}

void RequestParser::handleHeaderContentLength(Request& req, const headersMap& headers) {
    std::string contentLengthHeader;
    size_t      contentLength;

    if (req.hasHeader(TRANSFER_ENCODING)) {
        req.setStatusCode(BAD_REQUEST);
        throw RequestParsingError(
            "handleHeaderContentLength: a request cannot have both content-length and transfer-encoding headers");
    }

    contentLengthHeader = headers.at(CONTENT_LENGTH);
    if (contentLengthHeader.find(",") != std::string::npos) {
        req.setStatusCode(BAD_REQUEST);
        throw RequestParsingError("handleHeaderContentLength: header can only appear once in the request");
    }

    std::stringstream s(contentLengthHeader);
    int               c;
    while ((c = s.get()) != EOF) {
        if (!std::isdigit(static_cast<unsigned char>(c)))
            throw RequestParsingError("handleHeaderContentLength: header {" + contentLengthHeader +
                                      "} contains non-digit characters");
    }

    contentLength = toSizet(req.getHeader(CONTENT_LENGTH));
    if (contentLength > static_cast<size_t>(_maxBodySize)) { // TEST THIS BY MODIFYING CONFIG
        req.setStatusCode(CONTENT_TOO_LARGE);
        throw RequestParsingError("handleHeaderContentLength: exceeds _maxBodySize (" + toString(_maxBodySize) + ")");
    }

    _contentLength = contentLength;
}

void RequestParser::validateHeaders(Request& req) {
    const headersMap headers = req.getHeaders();

    _maxBodySize = pow(16, 6); // temp value before choosing the correct serv

    // To do: get max body size from config before, we did:
    // const ServerConfig& serverConfig = _config.getServers().at(ctx.server_index);
    // size_t maxBodySize = serverConfig.client_max_body_size;

    if (!req.hasHeader(HOST) || // TEST THIS
        headers.at(HOST).find(",") != std::string::npos) {
        req.setStatusCode(BAD_REQUEST);
        throw RequestParsingError("parseHeaders: request must have exactly one host header");
    }

    if ((req.getMethod() == GET || req.getMethod() == DELETE) &&
        (req.hasHeader(CONTENT_LENGTH) || req.hasHeader(TRANSFER_ENCODING))) {
        std::string error_msg = "parseHeaders: a request of type " + methodToString(req.getMethod()) +
                                " cannot have a content-length header";
        throw RequestParsingError(error_msg.c_str());
    }

    if (req.hasHeader(CONTENT_LENGTH))
        handleHeaderContentLength(req, headers);

    if (req.hasHeader(TRANSFER_ENCODING) &&
        headers.at(TRANSFER_ENCODING) != "chunked") { // the transfer-encoding header value must be "chunked"
        req.setStatusCode(NOT_IMPLEMENTED);
        throw RequestParsingError("parseHeaders: transfer-encoding can only have value 'chunked'");
    }
}

// compare header name to supported header to see if there is a field value
// 	=> if yes, append to existing value with ","
// 	=> if no, append to existing (empty) value
void RequestParser::fillHeadersMap(std::pair<std::string, std::string> const& header_pair, Request& req) {
    std::string headerName  = header_pair.first;
    std::string headerField = header_pair.second;
    std::string existingHeader;

    initHeaderStringToEnumMap();
    if (_headerStringToEnum.find(headerName) == _headerStringToEnum.end())
        return;
    existingHeader = req.getHeader(_headerStringToEnum[headerName]);
    if (!existingHeader.empty())
        headerField = "," + headerField; // add a comma if there is already a value for a given header
    req.setHeader(_headerStringToEnum[headerName], headerField);
}

void RequestParser::initHeaderStringToEnumMap(void) {
    _headerStringToEnum["host"]              = HOST;
    _headerStringToEnum["content-length"]    = CONTENT_LENGTH;
    _headerStringToEnum["location"]          = LOCATION;
    _headerStringToEnum["transfer-encoding"] = TRANSFER_ENCODING;
    _headerStringToEnum["content-type"]      = CONTENT_TYPE;
    _headerStringToEnum["connection"]        = CONNECTION;
    _headerStringToEnum["accept"]            = ACCEPT;
}

bool RequestParser::isCaseInsensitiveHeader(std::string& headerName) {
    if (headerName == "host" || headerName == "content-length" || headerName == "transfer-encoding" ||
        headerName == "content-type")
        return true;
    return false;
}
