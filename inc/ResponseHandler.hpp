// ResponseHandler.hpp

#include "fstream"

class ResponseHandler {
  private:
  public:
    ResponseHandler();
    ~ResponseHandler();

    void ResponseHandler::handleGet(Request& req);
    void handlePost();
    void handleDelete();
};
