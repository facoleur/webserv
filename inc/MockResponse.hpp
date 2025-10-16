// MockResponse.hpp

#pragma once

#include <iostream>
#include <map>
#include <utility>

class MockResponse {
private:
  std::string response;

public:
  MockResponse();
  ~MockResponse();

  std::string getResponse() const;
};

std::string MockResponse::getResponse() const { return response; }

MockResponse::MockResponse() {
  // chatgpt mock response
  response = "HTTP/1.1 200 OK\r\n"
             "Content-Type: text/plain\r\n"
             "Content-Length: 12\r\n"
             "Connection: close\r\n"
             "\r\n"
             "Hello world!";
}

MockResponse::~MockResponse() {}
