#include <iostream>

#include <pulse/pulseaudio.h>

int main() {
  std::cout<<"Let's test that linkage is all good" << std::endl;
  std::cout<<"Calling pa_msleep(3000)" << std::endl;
  pa_msleep(3000);
  std::cout << "Back up, bye!" << std::endl;
}
