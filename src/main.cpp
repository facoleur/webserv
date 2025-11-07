// main.cpp

#include "ConfigFile.hpp"
#include "ConfigParser.hpp"
#include "Server.hpp"
#include <iostream>
#include <unistd.h>

int main(int argc, char const* argv[]) {
    const char* path = (argc > 1) ? argv[1] : "config/default.conf";
    // Validate config path early
    std::string err;
    if (!ConfigFile::validateConfigPath(path, err)) {
        std::cerr << err << "\n";
        return 1;
    }
    ConfigParser parser;
    Config       cfg = parser.parseFile(path);
    applyDefaults(cfg);
    validateCompatibility(cfg);

    Server serv(cfg);
    serv.run();
    return 0;
}

///////////////////////////////////

// #include "RequestRouter.hpp"
// #include <iostream>
// #include <stdexcept>
// #include <string>
// #include <vector>

// // === minimal Request mock ===

// // === mock RequestRouter for testing ===

// // Copy-paste the resolvePath implementation you wrote earlier here

// // === test runner ===
// int main() {
//     RequestRouter router;
//     std::string   root = "../www";

//     struct TestCase {
//         std::string input;
//         std::string expected;
//         bool        shouldThrow;
//     };

//     TestCase tests[] = {
//         {"http://example.com/index.html", "../www/index.html", false},
//         {"/index.html", "../www/index.html", false},
//         {"/foo/bar.txt", "../www/foo/bar.txt", false},
//         {"/foo//bar", "../www/foo/bar", false},
//         {"/foo///bar/////////////", "../www/foo/bar/", false},
//         {"/foo/../bar.txt", "", true},
//         {"/../etc/passwd", "", true},
//         {"/foo%2e%2e/bar", "", true},
//         {"/path/file?x=1&y=2", "", true},
//         {"/foo=bar", "", true},
//         {"/", "../www/", false},
//     };

//     const int numTests = sizeof(tests) / sizeof(TestCase);
//     for (int i = 0; i < numTests; ++i) {
//         const TestCase& t = tests[i];
//         Request         req;
//         req.setPath(t.input);
//         std::cout << "Test " << (i + 1) << ": \"" << t.input << "\" -> ";
//         try {
//             std::string result = router.resolvePath(req, root);
//             if (t.shouldThrow) {
//                 std::cout << "❌ expected exception, got \"" << result << "\"\n";
//             } else {
//                 if (result == t.expected)
//                     std::cout << "✅ \"" << result << "\"\n";
//                 else
//                     std::cout << "❌ expected \"" << t.expected << "\", got \"" << result << "\"\n";
//             }
//         } catch (const std::exception& e) {
//             if (t.shouldThrow)
//                 std::cout << "✅ threw \"" << e.what() << "\"\n";
//             else
//                 std::cout << "❌ unexpected exception: " << e.what() << "\n";
//         } catch (...) {
//             std::cout << "❌ unknown exception\n";
//         }
//     }

//     return 0;
// }
