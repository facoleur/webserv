// MockRequest.hpp

#pragma once

#include <iostream>
#include <map>
#include <utility>

enum requestMethod { GET, POST, DELETE, UNKNOWN };
enum requestValidity { VALID_REQUEST, INVALID_REQUEST };
enum requestHeaders {
  HOST,
  CONTENT_LENGTH,
  TRANSFER_ENCODING,
  CONTENT_TYPE,
  CONNECTION,
  ACCEPT
};

class MockRequest {
private:
  std::string path;
  std::string queryString;
  std::map<enum requestHeaders, std::string> headers;
  std::string body;
  int protocolVersion;

public:
  MockRequest();
  ~MockRequest();

  enum requestMethod getMethod(void);
  std::string &getPath(void);
  std::string &getQueryString(void);
  int getProtocolVersion(void);
  std::map<enum requestHeaders, std::string> &getHeaders(void);
  std::string &getBody(void);
  bool isRequestValid(void);
};

MockRequest::MockRequest() {
  path = "/about";
  queryString = "id=42";
  body = "mock body content";
  protocolVersion = 11;

  headers.insert(std::make_pair(HOST, "localhost:8080"));
  headers.insert(std::make_pair(CONTENT_LENGTH, "2000"));
  headers.insert(std::make_pair(TRANSFER_ENCODING, "chunked"));
  headers.insert(std::make_pair(CONTENT_TYPE, "text/html"));
  headers.insert(std::make_pair(CONNECTION, "keep-alive"));
  headers.insert(std::make_pair(ACCEPT, "*/*"));
}

MockRequest::~MockRequest() {}

enum requestMethod MockRequest::getMethod(void) { return GET; }

std::string &MockRequest::getPath(void) { return path; }

std::string &MockRequest::getQueryString(void) { return queryString; }

int MockRequest::getProtocolVersion(void) { return 11; }

std::map<enum requestHeaders, std::string> &MockRequest::getHeaders(void) {
  return headers;
}

std::string &MockRequest::getBody(void) { return body; }

bool MockRequest::isRequestValid(void) { return true; }
