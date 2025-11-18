#include "Request.hpp"
#include "RequestRouter.hpp"
#include "Response.hpp"
#include <fstream>
#include <iostream>
#include <limits.h>
#include <sys/stat.h>
#include <unistd.h>

class TestRouter : public RequestRouter {
  public:
    using RequestRouter::handlePost;
};

Request makeRequest(const std::string& path, const std::string& body) {
    Request req;
    req.setMethod("POST");
    req.setPath(path);
    req.setBody(body);
    return req;
}

LocationConfig makeConfig(const std::string& uploadDir) {
    LocationConfig cfg;
    cfg.upload_enable = true;
    cfg.upload_store  = uploadDir;
    return cfg;
}

std::string readFile(const std::string& filename) {
    std::ifstream in(filename.c_str(), std::ios::binary);
    std::string   data;
    char          buf[1024];

    while (in.read(buf, sizeof(buf)))
        data.append(buf, sizeof(buf));
    data.append(buf, in.gcount());

    return data;
}

static void mkdirRecursive(const std::string& path) {
    std::string cur;
    for (size_t i = 0; i < path.size(); i++) {
        cur.push_back(path[i]);
        if (path[i] == '/' && cur.size() > 1) {
            mkdir(cur.c_str(), 0777);
        }
    }
    mkdir(path.c_str(), 0777);
}

std::string makeBinary256() {
    std::string s;
    s.reserve(256);
    for (int i = 0; i < 256; i++)
        s.push_back(static_cast<char>(i));
    return s;
}

std::string makeBig() {
    return std::string(50000, 'A');
}

int main() {
    TestRouter router;

    const std::string uploadDir = "test_uploads";
    mkdir(uploadDir.c_str(), 0777);

    struct TC {
        std::string path;
        std::string body;
        std::string filename;
    };

    TC tests[] = {
        {"/upload/simple.txt", "hello world", "simple.txt"},
        {"/upload/empty.txt", "", "empty.txt"},
        {"/upload/bin1.bin", std::string("\x00\x01\x02\x03\x04\x00\xFF", 7), "bin1.bin"},
        {"/upload/full256.bin", makeBinary256(), "full256.bin"},
        {"/upload/newlines.txt", "line1\nline2\r\nline3\n", "newlines.txt"},
        {"/upload/img.jpg", "JPEGDATAHERE", "img.jpg"},
        {"/upload/large.dat", makeBig(), "large.dat"},
        {"/upload/simple.txt", "overwrite content", "simple.txt"},
    };

    int count = sizeof(tests) / sizeof(TC);

    for (int i = 0; i < count; i++) {
        const TC& tc = tests[i];

        Request        req = makeRequest(tc.path, tc.body);
        LocationConfig cfg = makeConfig(uploadDir);

        // ensure nested directories exist
        size_t slash = tc.filename.find_last_of('/');
        if (slash != std::string::npos) {
            std::string nested = uploadDir + "/" + tc.filename.substr(0, slash);
            mkdirRecursive(nested);
        }

        Response res = router.handlePost(req, tc.path, cfg);

        std::string filepath = uploadDir + "/" + tc.filename;
        std::string saved    = readFile(filepath);

        bool ok = (saved == tc.body);

        if (ok) {
            std::cout << "OK: " << filepath << std::endl;
        } else {
            std::cout << "FAILED: " << filepath << std::endl;
            std::cout << "expected size: " << tc.body.size() << " got: " << saved.size() << std::endl;

            std::cout << "first differing byte at index: ";
            size_t minSize = std::min(saved.size(), tc.body.size());
            size_t diff    = minSize;
            for (size_t j = 0; j < minSize; j++) {
                if (saved[j] != tc.body[j]) {
                    diff = j;
                    break;
                }
            }
            if (diff == minSize)
                std::cout << "none, but size mismatch" << std::endl;
            else
                std::cout << diff << std::endl;
        }
    }

    std::cout << "Tests completed" << std::endl;

    return 0;
}
