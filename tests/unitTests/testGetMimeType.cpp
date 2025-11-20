#include "Config.hpp"
#include "Request.hpp"
#include "RequestRouter.hpp"
#include "Response.hpp"

class TestRequestRouter : public RequestRouter {
  public:
    using RequestRouter::getMimeType;
};

struct TestCase {
    std::string path;
    std::string expected;

    TestCase(std::string path, std::string expected) : path(path), expected(expected) {
    }
};

int main() {
    TestRequestRouter router;

    std::vector<TestCase> paths;

    paths.push_back(TestCase("www/dir/", "text/plain"));
    paths.push_back(TestCase("www/dir/index.html", "text/html"));
    paths.push_back(TestCase("www/dir/img.png", "image/png"));
    paths.push_back(TestCase("www/dir/img.", "text/plain"));
    paths.push_back(TestCase("www/dir/img...jpg", "image/jpeg"));
    paths.push_back(TestCase("www/dir/img...", "text/plain"));
    paths.push_back(TestCase("www/dir/PICTURE.JPG", "image/jpeg"));
    paths.push_back(TestCase("www/dir/Document.HTML", "text/html"));
    paths.push_back(TestCase("www/dir/style.CsS", "text/css"));
    paths.push_back(TestCase("www/dir/script.Js", "application/javascript"));
    paths.push_back(TestCase("", "text/plain"));

    for (uint i = 0; i < paths.size(); i++) {
        std::string mime = router.getMimeType(paths[i].path);
        if (mime == paths[i].expected)
            std::cout << "OK: ";
        else
            std::cout << "PAS OK: ";
        std::cout << paths[i].path << ": " << mime << std::endl;
    }

    return 0;
}
