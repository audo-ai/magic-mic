#include <iostream>
#include <string>

#include "app.h"

int main(int argc, char **argv) {
  try {
    std::cout << "Starting app" << std::endl;
    App app;
    app.run();
  } catch (std::string &s) {
    std::cout << "App failed with error: \"" << s << "\"" << std::endl;
  }
}
