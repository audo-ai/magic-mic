#include <iostream>
#include <thread>
#include <csignal>

#include <virtual_mic.h>

#ifdef USE_PIPESOURCE
#include <pipesource_virtual_mic.h>
typedef PipeSourceVirtualMic ConcreteVirtualMic;
#define VIRTUAL_MIC_NAME "PIPESOURCE"
#endif

static volatile bool running;

void handle_signal(int sig) {
  std::cout << "Signal Recived" << std::endl;
  switch (sig) {
  case SIGTERM:
  case SIGINT:
    running = false;
    break;
  }
}

int main () {
  signal(SIGINT, handle_signal);
  std::cout << "Starting " << VIRTUAL_MIC_NAME << " virtual mic" << std::endl;
  ConcreteVirtualMic mic;
  running = true;
  while (running) {
    std::this_thread::yield();
  }
  mic.stop();
  return 0;
}
