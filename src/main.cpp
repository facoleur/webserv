// main.cpp

#include "Server.hpp"

int main(int argc, char const *argv[]) {

  (void)argc;
  (void)argv;

  Config conf;

  Server serv(conf);

  serv.run();
  return 0;
}
