
#include <iostream>
#include <map>
#include <sstream>
#include <string>

struct RequestParsingError : std::exception {};

// #ifdef DEBUG_MODE
#define DEBUG_LOG(x) std::cout << x << std::endl
// #else
// #define DEBUG_LOG(x)
// #endif

bool isSpace(int i) {
    return (std::isspace(i));
}

std::string toString(long long n) {

    std::stringstream ss;
    ss << n;
    return ss.str();
}

std::string toLower(const std::string& s) {
    std::string out = s;
    for (size_t i = 0; i < out.size(); i++)
        out[i] = std::toLower(static_cast<unsigned char>(out[i]));
    return out;
}

// trims whitespace at the beginning and end of a std::string
void trimWhitespace(std::string& headerField) {
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

bool isCaseInsensitiveHeader(std::string& headerName) {
    if (headerName == "host" || headerName == "content-length" || headerName == "transfer-encoding" ||
        headerName == "content-type")
        return true;
    return false;
}

// RFC 7230#3.2:
// header-field   = field-name ":" OWS field-value OWS. OWS: optional whitespace
void checkHeaderSyntax(std::string& header) {
    std::string::difference_type n;
    int                          count;
    size_t                       pos;
    std::string::iterator        it;
    std::string                  headerName;
    std::string                  headerField;

    // check header size
    if (header.size() > 400) {
        std::cout << "for header {" << header << "}, set status code to CONTENT_TOO_LARGE (413)" << std::endl;
        throw RequestParsingError();
    }

    // split on colon
    n     = std::count(header.begin(), header.end(), ':');
    count = static_cast<int>(n);
    if (count != 1) // not exactly one colon
        throw RequestParsingError();
    pos = header.find(":");
    if (pos == 0 || pos == header.size() - 1) // colon is first or last character
        DEBUG_LOG("colon found at beginning or end of header"), throw RequestParsingError();
    headerName  = header.substr(0, pos);
    headerField = header.substr(pos + 1); // skip the ":"
    // DEBUG_LOG("checkHeaderSyntax – headerName: {" + headerName + "}, headerField: {" + headerField + "}");

    // check if whitespace before colon
    it = std::find_if(headerName.begin(), headerName.end(), isSpace);
    if (it != headerName.end()) {
        DEBUG_LOG("whitespace found before colon");
        throw RequestParsingError();
    }

    // lowercase the headerName
    headerName = toLower(headerName);
    // std::transform(headerName.begin(), headerName.end(), headerName.begin(), toLowerChar);

    trimWhitespace(headerField);

    if (isCaseInsensitiveHeader(headerName))
        headerField = toLower(headerField);
}

#include <unistd.h>
int main(void) {
    char buf[1000];
    read(0, buf, 1000);

    std::string header(buf);
    std::string originalLine = header;

    try {
        checkHeaderSyntax(header);
        // After checkHeaderSyntax, `line` is still full header
        size_t      pos   = header.find(':');
        std::string name  = header.substr(0, pos);
        std::string value = header.substr(pos + 1);
        trimWhitespace(value);
        name = toLower(name);

        std::cout << "TEST OK" << std::endl;
        std::cout << "  -> name  = {" << name << "}" << std::endl;
        std::cout << "  -> value = {" << value << "}" << std::endl;
    } catch (const RequestParsingError&) {
        std::cout << "ERROR" << std::endl;
    }

    return 0;
}