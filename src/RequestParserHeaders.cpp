// RequestParserHeaders.cpp

#include <algorithm>

#include "Enums.hpp"
#include "Request.hpp"
#include "RequestParser.hpp"
#include "Utils.hpp"
#include "Webserv.hpp"

void RequestParser::parseHeaders(Request& req) {
    size_t      pos;
    std::string header;

    // example: 'Host: example.com\r\nFoo:  bar \r\n\r\n'
    pos = _headers.find(CRLF);
    while (pos != std::string::npos) {
        header   = _headers.substr(0, pos);
        _headers = _headers.substr(pos + 2);
        DEBUG_LOG("parseHeaders - header is {" + header + "}, and _headers is {" + _headers + "}");
        parseHeader(header, req);
        pos = _headers.find(CRLF);
    }
    if (_headers.size()) // last header
        header = _headers.substr(0, pos);
    DEBUG_LOG("parseHeaders - last header is {" + header + "}");
    parseHeader(header, req);
    _headers.clear();
}

void RequestParser::parseHeader(std::string& header, Request& req) {
    std::pair<std::string, std::string> header_pair;

    DEBUG_LOG("*** parseHeader ***");
    header_pair = checkHeaderSyntax(header, req);
    fillHeadersMap(header_pair, req);
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

    DEBUG_LOG("checkHeaderSyntax – header is {" + header + "}");

    // check header size
    if (header.size() < MIN_HEADER_SIZE)
        throw RequestParsingError("checkHeaderSyntax(): header < MIN_HEADER_SIZE");

    if (header.size() > MAX_HEADER_SIZE) {
        req.setStatusCode(CONTENT_TOO_LARGE);
        DEBUG_LOG("checkHeaderSyntax: ");
        throw RequestParsingError("checkHeaderSyntax(): header > MAX_HEADER_SIZE");
    }

    // split on colon
    n     = std::count(header.begin(), header.end(), ':');
    count = static_cast<int>(n);
    if (count != 1) // not exactly one colon
        throw RequestParsingError("checkHeaderSyntax(): header doesn't have exactly one colon (':')");

    pos = header.find(":");
    if (pos == 0 || pos == header.size() - 1) // colon is first or last character
        throw RequestParsingError("checkHeaderSyntax(): colon (':') is first or last character");

    headerName  = header.substr(0, pos);
    headerField = header.substr(pos + 1); // skip the ":"
    DEBUG_LOG("checkHeaderSyntax – headerName: {" + headerName + "}, headerField: {" + headerField + "}");

    // check if whitespace before colon
    it = std::find_if(headerName.begin(), headerName.end(), isSpace);
    DEBUG_LOG("checkHeaderSyntax – headerName is {" + headerName + "}");
    if (it != headerName.end()) {
        DEBUG_LOG("*it: " + toString(*it));
        throw RequestParsingError("checkHeaderSyntax(): whitespace found before colon (':')");
    }

    // format header name and field
    headerName = tolower(headerName);
    headerField = trimString(headerField);
    if (isCaseInsensitiveHeader(headerName))
        headerField = tolower(headerField);

    return std::pair<std::string, std::string>(headerName, headerField);
}

// compare header name to supported header to see if there is a field value
// 	=> if yes, append to existing value with ","
// 	=> if no, append to existing (empty) value
void RequestParser::fillHeadersMap(std::pair<std::string, std::string> const& header_pair, Request& req) {
    std::string headerName  = header_pair.first;
    std::string headerField = header_pair.second;
    std::string existingHeader;

    initHeaderStringToEnumMap();
    if (_headerStringToEnum.find(headerName) == _headerStringToEnum.end()) {
        DEBUG_LOG("headerName {" + headerName + "} not found in headers enum");
        return;
    }
    existingHeader = req.getHeader(_headerStringToEnum[headerName]);
    if (!existingHeader.empty())
        headerField = "," + headerField; // add a comma if there is already a value for a given header
    DEBUG_LOG("fillHeadersMap: headerField is {" + headerField + "}");
    req.setHeader(_headerStringToEnum[headerName], headerField);
#ifdef DEBUG_MODE
    std::cout << "fillHeadersMap: header {" << headerName << "} now has value {"
              << req.getHeader(_headerStringToEnum[headerName]) << "}" << std::endl;
    std::cout << req.getHeaders() << std::endl;
#endif
}

void RequestParser::validateHeaders(Request & req) const {
	const headersMap	headers = req.getHeaders();

    if (!req.hasHeader(HOST) || // TEST THIS
    headers.at(HOST).find(",") != std::string::npos) {
        req.setStatusCode(BAD_REQUEST);
        throw RequestParsingError("parseHeaders: request must have exactly one host header");
    }

    if (req.hasHeader(CONTENT_LENGTH) &&
	headers.at(CONTENT_LENGTH).find(",") != std::string::npos) {
        req.setStatusCode(BAD_REQUEST);
        throw RequestParsingError("parseHeaders: content-length header can only appear once in the request");
    }

    if (req.hasHeader(TRANSFER_ENCODING) &&
	headers.at(TRANSFER_ENCODING) != "chunked") { // the transfer-encoding header value must be "chunked"
        req.setStatusCode(NOT_IMPLEMENTED);
        throw RequestParsingError("parseHeaders: transfer-encoding can only have value 'chunked'");
    }

    if (req.hasHeader(CONTENT_LENGTH) && req.hasHeader(TRANSFER_ENCODING)) {
        req.setStatusCode(BAD_REQUEST); // ?
        throw RequestParsingError("parseHeaders: a request cannot have both content-length and transfer-encoding headers");
    }
}
