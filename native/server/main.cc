#include <csignal>
#include <iostream>
#include <sstream>
#include <thread>

#include <errno.h>
#include <fcntl.h>
#include <sys/select.h>
#include <unistd.h>

#include <nlohmann/json.hpp>

#include <virtual_mic.h>

#ifdef USE_PIPESOURCE
#include <pipesource_virtual_mic.h>
typedef PipeSourceVirtualMic ConcreteVirtualMic;
#define VIRTUAL_MIC_NAME "PIPESOURCE"
#endif

#include "rpc.h"

using std::stringstream;
using json = nlohmann::json;

static bool running = true;
void handle_signal(int sig) {
  std::cout << "Signal Recived" << std::endl;
  switch (sig) {
  case SIGTERM:
  case SIGINT:
    running = false;
    break;
  }
}
constexpr char delim = '\n';
int main() {
  signal(SIGINT, handle_signal);
  std::cout << "Starting " << VIRTUAL_MIC_NAME << " virtual mic" << std::endl;
  // ConcreteVirtualMic mic;
  fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);

  stringstream ss;
  while (running) {
    fd_set rfds;
    struct timespec tv;
    sigset_t mask;
    int ret;
    FD_ZERO(&rfds);
    FD_SET(STDIN_FILENO, &rfds);
    tv.tv_sec = 1;
    tv.tv_nsec = 0;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);

    ret = pselect(STDIN_FILENO + 1, &rfds, nullptr, nullptr, &tv, &mask);
    if (ret == -1) {
      std::cerr << "pselect error: " << strerror(errno) << std::endl;
      running = false;
    }

    if (FD_ISSET(STDIN_FILENO, &rfds)) {
      char line[1024] = {};
      char c;
      // long read_in = read(STDIN_FILENO, line, sizeof(line));
      while (1 == read(STDIN_FILENO, &c, 1)) {
	// Making the assumption that the last read char of a block will always
	// be the delimiter. This is valid with a newline delimilter in an
	// interactive terminal, but not necessarily with an arbitrary
	// delimmiter on a non terminal. For that, it would probably be best to
	// read char by char
	if (c == delim) {
	  json j;
	  ss << line;
	  try {
	    ss >> j;
	    RPCRequest req = parse_request(j);
	    std::cerr << "Handling: " << req.type << std::endl;
	  } catch (nlohmann::detail::parse_error &ex) {
	    std::cout << make_error(-32700, "Parse error") << std::endl;
	  } catch (json err) {
	    std::cout << err << std::endl;
	  }
	  ss.str("");
        } else {
          ss << c;
        }
      }
    }
  }
  // mic.stop();
  return 0;
}
