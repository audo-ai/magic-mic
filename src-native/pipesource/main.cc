#include <iostream>
#include <string>

#include "app.h"

int main(int argc, char **argv) {
  try {
    std::cout << "Starting app" << std::endl;
    PipesourceVirtualMic app;
    app.run();
  } catch (std::string &s) {
    std::cerr << "App failed with error: \"" << s << "\"" << std::endl;
    return -1;
  }
  return 0;
}
