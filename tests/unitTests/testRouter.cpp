#include "Config.hpp"
#include "ConfigParser.hpp"
#include "Request.hpp"
#include "RequestRouter.hpp"
#include "Response.hpp"
#include "Utils.hpp"
#include <iostream>
#include <string>
#include <vector>

struct TestCase {
    std::string path;
    std::string method;
    int         expectedStatus;
    std::string expectedReason;

    TestCase(const std::string& p, const std::string& m, int status, const std::string& reason)
        : path(p), method(m), expectedStatus(status), expectedReason(reason) {
    }
};

void printResult(int i, const TestCase& t, const Response& res, bool ok, const std::string& msg = "") {
    (void)res;
    if (ok)
        std::cout << "OK ";
    else
        std::cout << "PAS OK " << msg;
    std::cout << "TEST " << (i + 1) << ": " << t.method << " " << t.path << std::endl;
}

int main() {

    RequestRouter router;
    // ServerConfig  config = makeTestConfig();

    ConfigParser parser;
    Config       sconfig = parser.parseFile("/home/facoleur/Documents/webserv/config/default.conf");
    ServerConfig config  = sconfig.getServers()[0];

    std::vector<TestCase> tests;

    // ---------- GET ----------
    tests.push_back(TestCase("/index.html", "GET", 200, "OK"));
    tests.push_back(TestCase("/missing.html", "GET", 404, "Not Found"));
    tests.push_back(TestCase("/", "GET", 200, "OK"));
    tests.push_back(TestCase("/dir/", "GET", 200, "OK"));
    tests.push_back(TestCase("/donotexist/", "GET", 404, "Not Found"));
    tests.push_back(TestCase("/../hack", "GET", 400, "Bad Request"));
    tests.push_back(TestCase("/redirect", "GET", 301, "Redirect"));

    // ---------- CGI ----------
    tests.push_back(TestCase("/cgi-bin/cgi.py", "GET", 200, "OK"));
    tests.push_back(TestCase("/cgi-bin/error.py", "GET", 500, "Internal Server Error"));
    tests.push_back(TestCase("/cgi-bin/echo.py", "POST", 200, "OK")); // CGI POST

    // ---------- POST ----------
    tests.push_back(TestCase("/upload", "POST", 201, "Created"));                   // Normal POST (create)
    tests.push_back(TestCase("/upload/limited", "POST", 413, "Payload Too Large")); // Too big body
    tests.push_back(TestCase("/missing-endpoint", "POST", 404, "Not Found"));       // Invalid endpoint
    tests.push_back(TestCase("/../upload", "POST", 400, "Bad Request"));            // Directory traversal

    // ---------- DELETE ----------
    tests.push_back(TestCase("/delete/file.txt", "DELETE", 204, "No Content"));            // Valid delete
    tests.push_back(TestCase("/getonly/index.html", "DELETE", 405, "Method Not Allowed")); // Disallowed
    tests.push_back(TestCase("/nonexistent", "DELETE", 404, "Not Found"));                 // Missing file
    tests.push_back(TestCase("/../etc/passwd", "DELETE", 400, "Bad Request"));             // Traversal attack

    // ---------- UNKNOWN METHODS ----------
    tests.push_back(TestCase("/index.html", "PATCH", 501, "Not Implemented"));   // Unsupported verb
    tests.push_back(TestCase("/index.html", "BREW", 501, "Not Implemented"));    // Random method
    tests.push_back(TestCase("/index.html", "PUT", 501, "Not Implemented"));     // Unhandled verb
    tests.push_back(TestCase("/index.html", "CONNECT", 501, "Not Implemented")); // Unsupported standard
    tests.push_back(TestCase("/index.html", "TRACE", 501, "Not Implemented"));   // Disabled trace

    std::cout << "\n=== Route Tests ===\n";

    std::ofstream out("www/delete/file.txt");

    for (size_t i = 0; i < tests.size(); ++i) {
        const TestCase& t = tests[i];
        Request         req;
        req.setPath(t.path);
        req.setMethod(t.method);
        std::string pv = "HTTP/1.1";
        req.setProtocolVersion(pv);
        std::string body = "this is a body";
        req.setHeader(CONTENT_LENGTH, toString(body.size()));
        req.setBody(body);

        Response    res;
        bool        ok = false;
        std::string msg;

        try {
            res = router.route(req, config);

            if (res.getStatusCode() == t.expectedStatus && ReasonPhrase::get(res.getStatusCode()) == t.expectedReason) {
                ok = true;
            } else {
                msg = "expected " + toString(t.expectedStatus) + " (" + t.expectedReason + "), got " +
                      toString(res.getStatusCode()) + " (" + ReasonPhrase::get(res.getStatusCode()) + ")";
            }
        } catch (const std::exception& e) {
            msg = std::string("unexpected exception: ") + e.what();
        }

        printResult(i, t, res, ok, msg);
    }

    return 0;
}
