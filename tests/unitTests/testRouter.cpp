#include "Config.hpp"
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
    bool        shouldThrow;

    TestCase(const std::string& p, const std::string& m, int status, const std::string& reason, bool throwFlag)
        : path(p), method(m), expectedStatus(status), expectedReason(reason), shouldThrow(throwFlag) {
    }
};

void printResult(int i, const TestCase& t, const Response& res, bool ok, const std::string& msg = "") {
    (void)res;
    if (ok)
        std::cout << "OK";
    else
        std::cout << "PAS OK" << msg;
    std::cout << "TEST " << (i + 1) << ": " << t.method << " " << t.path << std::endl;
}

static ServerConfig makeTestConfig() {
    ServerConfig config;
    config.root = "www";
    config.index_files.push_back("index.html");
    config.autoindex = true;
    config.methods.insert(GET);

    LocationConfig rootLoc;
    rootLoc.path = "/";
    rootLoc.methods.insert(GET);
    rootLoc.methods.insert(POST);
    rootLoc.autoindex = true;
    rootLoc.index_files.push_back("index.html");

    rootLoc.cgi_map["py"]  = "bin/py";
    rootLoc.cgi_map["sh"]  = "bin/sh";
    rootLoc.cgi_map["php"] = "bin/php";

    LocationConfig redirectLoc;
    redirectLoc.path            = "/redirect";
    redirectLoc.redirect.status = 301;
    redirectLoc.redirect.target = "/index.html";

    redirectLoc.methods.insert(GET);

    config.locations.push_back(rootLoc);
    config.locations.push_back(redirectLoc);

    return config;
}

int main() {

    RequestRouter router;
    ServerConfig  config = makeTestConfig();

    std::vector<TestCase> tests;

    // ---------- GET ----------
    tests.push_back(TestCase("/index.html", "GET", 200, "OK", false));                          // Normal GET
    tests.push_back(TestCase("/missing.html", "GET", 404, "Not Found", false));                 // Missing file
    tests.push_back(TestCase("/", "GET", 200, "OK", false));                                    // Root directory
    tests.push_back(TestCase("/dir/", "GET", 200, "OK", false));                                // Directory with
    tests.push_back(TestCase("/private/secret.txt", "GET", 403, "Forbidden", false));           // Forbidden
    tests.push_back(TestCase("/../hack", "GET", 0, "", true));                                  // Traversal
    tests.push_back(TestCase("/redirect", "GET", 301, "Moved Permanently", false));             // Redirect
    tests.push_back(TestCase("/cgi-bin/cgi.py", "GET", 200, "OK", false));                      // CGI GET
    tests.push_back(TestCase("/cgi-bin/error.py", "GET", 500, "Internal Server Error", false)); // Broken CGI

    // // ---------- POST ----------
    // tests.push_back(TestCase("/api/upload", "POST", 201, "Created", false));           // Normal POST (create)
    // tests.push_back(TestCase("/api/upload", "POST", 413, "Payload Too Large", false)); // Too big body
    // tests.push_back(TestCase("/cgi-bin/echo.py", "POST", 200, "OK", false));           // CGI POST
    // tests.push_back(TestCase("/readonly/file.txt", "POST", 403, "Forbidden", false));  // Not allowed location
    // tests.push_back(TestCase("/missing-endpoint", "POST", 404, "Not Found", false));   // Invalid endpoint
    // tests.push_back(TestCase("/../upload", "POST", 0, "", true));                      // Directory traversal
    // tests.push_back(TestCase("/api/json", "POST", 200, "OK", false));                  // API JSON POST

    // // ---------- DELETE ----------
    // tests.push_back(TestCase("/index.html", "DELETE", 405, "Method Not Allowed", false));  // Disallowed
    // tests.push_back(TestCase("/api/resource", "DELETE", 204, "No Content", false));        // Valid delete
    // tests.push_back(TestCase("/protected/config.cfg", "DELETE", 403, "Forbidden", false)); // Protected file
    // tests.push_back(TestCase("/nonexistent", "DELETE", 404, "Not Found", false));          // Missing file
    // tests.push_back(TestCase("/../etc/passwd", "DELETE", 0, "", true));                    // Traversal attack

    // // ---------- UNKNOWN METHODS ----------
    // tests.push_back(TestCase("/index.html", "PATCH", 405, "Method Not Allowed", false)); // Unsupported verb
    // tests.push_back(TestCase("/index.html", "BREW", 400, "Bad Request", false));         // Random method
    // tests.push_back(TestCase("/index.html", "PUT", 405, "Method Not Allowed", false));   // Unhandled verb
    // tests.push_back(TestCase("/index.html", "CONNECT", 501, "Not Implemented", false));  // Unsupported standard
    // tests.push_back(TestCase("/index.html", "TRACE", 405, "Method Not Allowed", false)); // Disabled trace

    tests.push_back(TestCase("/dirwithoutslash/", "GET", 200, "OK", false));      // Disabled trace
    tests.push_back(TestCase("/dirwithoutslash", "GET", 301, "Redirect", false)); // Disabled trace
    tests.push_back(TestCase("/redirect", "GET", 301, "Redirect", false));        // Disabled trace
    tests.push_back(TestCase("/redirect/", "GET", 301, "Redirect", false));       // Disabled trace

    std::cout << "\n=== Route Tests ===\n";

    for (size_t i = 0; i < tests.size(); ++i) {
        const TestCase& t = tests[i];
        Request         req;
        req.setPath(t.path);
        req.setMethod(t.method);

        Response    res;
        bool        ok = false;
        std::string msg;

        try {
            res = router.route(req, config);

            if (t.shouldThrow) {
                msg = "expected exception, got status " + toString(res.getStatusCode());
            } else if (res.getStatusCode() == t.expectedStatus &&
                       ReasonPhrase::get(res.getStatusCode()) == t.expectedReason) {
                ok = true;
            } else {
                msg = "expected " + toString(t.expectedStatus) + " (" + t.expectedReason + "), got " +
                      toString(res.getStatusCode()) + " (" + ReasonPhrase::get(res.getStatusCode()) + ")";
            }
        } catch (const std::exception& e) {
            if (t.shouldThrow) {
                ok  = true;
                msg = std::string("threw expected exception: ") + e.what();
            } else {
                msg = std::string("unexpected exception: ") + e.what();
            }
        } catch (...) {
            msg = "unknown exception";
        }

        printResult(i, t, res, ok, msg);
    }

    std::cout << "\nAll tests completed.\n";

    return 0;
}
