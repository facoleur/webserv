// ResponseHandler.cpp

#include "ResponseHandler.hpp"
#include "Request.hpp"
#include "Response.hpp"

ResponseHandler::ResponseHandler() {
}

ResponseHandler::~ResponseHandler() {
}

void ResponseHandler::handleGet(Request& req) {
    Response response;

    std::string root = "./www";
    std::string path = req.getPath();

    std::ifstream file(path.c_str());
}
