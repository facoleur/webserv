// main.cpp

#include "Server.hpp"

int main(int argc, char const *argv[]) {

  Config conf;

  Server serv(conf);

  serv.run();
  return 0;
}
