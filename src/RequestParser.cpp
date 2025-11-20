// RequestParser.cpp

#include <algorithm>
#include <cctype>

#include "RequestParser.hpp"
#include "Server.hpp"

RequestParser::RequestParser(void)
    : _parserState(REQ_PARSE_START), _parsingPhase(PARSING_REQUEST_LINE), _statusCode(NO_STATUS), _contentLength(0),
      _accumulator(), _firstSection(), _requestLine(), _headers(), _body() {
}

RequestParser::~RequestParser(void) {
}

void RequestParser::initHeaderStringToEnumMap(void) {
    _headerStringToEnum["Host"]              = HOST;
    _headerStringToEnum["Content-Length"]    = CONTENT_LENGTH;
    _headerStringToEnum["Location"]          = LOCATION;
    _headerStringToEnum["Transfer-Encoding"] = TRANSFER_ENCODING;
    _headerStringToEnum["Content-Type"]      = CONTENT_TYPE;
    _headerStringToEnum["Connection"]        = CONNECTION;
    _headerStringToEnum["Accept"]            = ACCEPT;
}

ParserState RequestParser::getState(void) {
    return _parserState;
}

void RequestParser::setState(ParserState parserState) {
    _parserState = parserState;
}

// splits the line in three. Throws if less than two spaces found
void RequestParser::splitRequestLine(std::vector<std::string>& split, std::string& line) {
    size_t pos;

    for (size_t i = 0; i < 2; i++) {
        pos = line.find(' ');
        if (pos == line.npos)
            throw RequestParsingError("splitRequestLine(): found less than two spaces");
        split.push_back(line.substr(0, pos));
        line = line.substr(pos + 1);
    }
    split.push_back(line.substr(0));
    return;
}

void RequestParser::parseRequestLine(Request& req) {
    std::vector<std::string> split;
    size_t                   queryPos;

    /* split line */
    splitRequestLine(split, _requestLine);

    /* set method */
    if (split[0].empty())
        throw RequestParsingError("parseRequestLine(): method field is empty");

    req.setMethod(split[0]);

    /* set request-target path and query-string */
    if (split[1].empty() || split[1].find_first_of(" \t\n\r\f\v") != std::string::npos)
        throw RequestParsingError("parseRequestLine(): request-target is empty or contains whitespace");
    queryPos = split[1].find("?");
    if (queryPos != std::string::npos) {
        req.setQueryString(split[1].substr(queryPos + 1));
        split[1] = split[1].substr(0, queryPos);
    }
    req.setPath(split[1]);

    /* set HTTP version */
    if (split[2] != "HTTP/1.0" && split[2] != "HTTP/1.1" && split[2] != "HTTP/0.9" && split[2] != "HTTP/2" &&
        split[2] != "HTTP/3")
        throw RequestParsingError("parseRequestLine(): HTTP version not in the list");
    req.setProtocolVersion(split[2]);
}

unsigned char RequestParser::toLowerChar(unsigned char c) {
    return static_cast<unsigned char>(std::tolower(c));
}

// trims whitespace at the beginning and end of a std::string
void RequestParser::trimWhitespace(std::string& headerField) {
    std::string::const_iterator begin = headerField.begin();
    std::string::const_iterator end   = headerField.end();

    // move begin forward while it points to whitespace
    while (begin != end && isSpace(static_cast<unsigned char>(*begin)))
        ++begin;

    // move end backward while it points to whitespace
    if (begin != end) {
        do {
            --end;
        } while (end != begin && isSpace(static_cast<unsigned char>(*end)));
        ++end; // move back to one-past-last non-space
    }
    headerField = std::string(begin, end);
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

    // check header size
    if (header.size() < MIN_HEADER_SIZE) {
        DEBUG_LOG("checkHeaderSyntax: ");
        throw RequestParsingError("checkHeaderSyntax(): header < MIN_HEADER_SIZE");
    }

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
    if (it != header.end())
        throw RequestParsingError("checkHeaderSyntax(): whitespace found before colon (':')");

    // format header name and field
    headerName = tolower(headerName);
    trimWhitespace(headerField);
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
    if (_headerStringToEnum[headerName] > NB_REQUEST_HEADERS)
        return;
    existingHeader = req.getHeader(_headerStringToEnum[headerName]);
    if (!existingHeader.empty())
        headerField = "," + headerField; // add a comma if there is already a value for a given header
    req.setHeader(_headerStringToEnum[headerName], headerField);
#ifdef DEBUG_LOG
    std::cout << "fillHeadersMap: header {" << headerName << "} now has value {"
              << req.getHeader(_headerStringToEnum[headerName]) << "}" << std::endl;
    std::cout << req.getHeaders() << std::endl;
#endif
}

void RequestParser::parseHeader(std::string& header, Request& req) {
    std::pair<std::string, std::string> header_pair;

    header_pair = checkHeaderSyntax(header, req);
    fillHeadersMap(header_pair, req);
}

void RequestParser::parseHeaders(Request& req) {
    size_t      pos;
    std::string header;

    pos = _headers.find(CRLF);
    while (pos != std::string::npos) {
        header   = _headers.substr(0, pos);
        _headers = _headers.substr(pos + 1);
        parseHeader(header, req);
        pos = _headers.find(CRLF);
    }
    if (_headers.size()) // last header
        parseHeader(header, req);
    _headers.clear();
}

void RequestParser::handleParseError(Request& req, std::queue<Request>& reqQueue) {
    DEBUG_LOG("Parse error");
    if (req.getStatusCode() == NO_STATUS)
        req.setStatusCode(BAD_REQUEST);
    req.setValidity(INVALID_REQUEST);
    reqQueue.push(req);
    _parserState = REQ_PARSE_ERROR;
}

void RequestParser::feed(char* buf, std::queue<Request>& reqQueue) {
    size_t  pos;
    Request req;

    _accumulator += buf;
    while (!_accumulator.empty()) {
        /* 1. extract the content */
        switch (_parsingPhase) {
            case PARSING_REQUEST_LINE:
            case PARSING_HEADERS:
                pos = _accumulator.find(CRLF + CRLF);
                if (pos == std::string::npos) {
                    _firstSection += _accumulator; // .substr(0, pos)
                    if (_firstSection.size() >= READ_BUF_SIZE)
                        return handleParseError(req, reqQueue);
                    _accumulator.clear();
                    _parserState = REQ_PARSE_PARTIAL;
                    return;
                } else {
                    _firstSection += _accumulator.substr(0, pos);
                    if (_firstSection.size() >= READ_BUF_SIZE || _firstSection.size() < MIN_REQ_SIZE) {
                        DEBUG_LOG("size: " + toString(_firstSection.size()) + " is below MIN_REQ_SIZE" +
                                  "\n_firstSection:" + _firstSection);
                        return handleParseError(req, reqQueue);
                    }
                    _accumulator = _accumulator.substr(pos + 4);
                }
                break;
            case PARSING_BODY:
                // ...
                break;
            case PARSING_COMPLETE:
                // ??
                break;
            default:
                break;
        }

        /* 2. parse the extracted content */
        try {
            if (_parsingPhase == PARSING_REQUEST_LINE) // internal
            {
                pos = _firstSection.find(CRLF);
                if (pos != std::string::npos) { // _firstSection has request-line + headers
                    _requestLine  = _firstSection.substr(0, pos);
                    _firstSection = _firstSection.substr(pos + 2);
                    parseRequestLine(req);
                    _parsingPhase = PARSING_HEADERS;
                } else { // _firstSection is a pure request-line with no headers => CHANGE THIS TO BAD REQUEST !
                    _requestLine = _firstSection;
                    _firstSection.clear();
                    parseRequestLine(req);
                    _parsingPhase = PARSING_COMPLETE;
                }
            }
            if (_parsingPhase == PARSING_HEADERS) {
                _headers = _firstSection;
                parseHeaders(req);
                if (req.hasHeader(CONTENT_LENGTH))
                    _parsingPhase = PARSING_BODY;
                else
                    _parsingPhase = PARSING_COMPLETE;
            }
            if (_parsingPhase == PARSING_BODY) {
                // 	parseBody();
                _parsingPhase = PARSING_COMPLETE;
            }
            if (_parsingPhase == PARSING_COMPLETE) {
                reqQueue.push(req);
                DEBUG_LOG("RequestParser::feed() parsed a request:");
                DEBUG_LOG(req);
                req           = Request();
                _parsingPhase = PARSING_REQUEST_LINE;
            }
        } catch (RequestParsingError& e) {
            DEBUG_LOG(e.what());
            return handleParseError(req, reqQueue);
        }
    }
    _parserState = REQ_PARSE_COMPLETE;
}
