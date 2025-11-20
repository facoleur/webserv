#include "Config.hpp"
#include "Request.hpp"
#include "RequestRouter.hpp"
#include "Response.hpp"

int main() {
    RequestRouter router;

    std::vector<std::string> paths;

    paths.push_back("www/dir/");

    router.makeAutoindexResponse(paths[0]);

    return 0;
}
