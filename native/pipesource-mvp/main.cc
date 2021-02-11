#include <iostream>
#include <string>

#include "app.h"

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cerr << "usage: " << argv[0] << " PIPE_SOURE_PATH" << std::endl;
    return -1;
  }
  try {
    std::cout << "Starting app" << std::endl;
    App app = App(std::string(argv[1]));
    app.run();
  } catch (std::string &s) {
    std::cerr << "App failed with error: \"" << s << "\"" << std::endl;
    return -1;
  }
  return 0;
}
