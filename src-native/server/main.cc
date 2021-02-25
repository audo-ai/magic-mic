#include <csignal>
#include <future>
#include <iostream>
#include <sstream>
#include <thread>
#include <string>
#include <vector>
#include <utility>
#include <exception>
#include <chrono>

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

using std::pair;
using std::promise;
using std::string;
using std::stringstream;
using std::vector;
using json = nlohmann::json;

static bool running = true;
constexpr char delim = '\n';

void handle_signal(int sig) {
  std::cerr << "Signal Recived" << std::endl;
  switch (sig) {
  case SIGTERM:
  case SIGINT:
    running = false;
    break;
  }
}
struct interrupted_error : public std::exception {};

template <typename T>
void wait_on_future(future<T> &f) {
  while (running) {
    if (std::future_status::ready == f.wait_for(std::chrono::milliseconds(100))) {
      return;
    }
  }
  throw interrupted_error();
}
json handle_request(ConcreteVirtualMic *mic, RPCRequest req) {
  switch (req.type) {
  case GetStatus: {
    future<bool> f = mic->getStatus();
    f.wait();
    return make_response(req.id, f.get());
  }
  case GetMicrophones: {
    future<vector<pair<int, string>>> f = mic->getMicrophones();
    try {
      wait_on_future<vector<pair<int, string>>>(f);
    } catch(interrupted_error &e) {
      return make_error(-32000, "Server error", "Interrupted while waiting for response");
    }
    json resp = json::basic_json::array({});
    for (auto &tuple : f.get()) {
      resp.push_back({{"id", std::get<0>(tuple)}, {"name", std::get<1>(tuple)}});
    }
    return make_response(req.id, resp);
  }
  case SetMicrophone: {
    future<void> f = mic->setMicrophone(req.mic_id);
    f.wait();
    return make_response(req.id, {});
  }
  case SetRemoveNoise: {
    future f = mic->setRemoveNoise(req.should_remove_noise);
    f.wait();
    return make_response(req.id, {});
  }
  default:
    throw std::runtime_error("Unknown request enountered in handle_request");
  }
}

int main() {
  signal(SIGINT, handle_signal);
  std::cerr << "Starting " << VIRTUAL_MIC_NAME << " virtual mic" << std::endl;
  ConcreteVirtualMic mic;
  auto err_fut = mic.get_exception_future();
  fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);

  stringstream ss;
  while (running) {
    if (std::future_status::ready == err_fut.wait_for(std::chrono::milliseconds(0))) {
      std::rethrow_exception(err_fut.get());
    }

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
      char c;
      while (1 == read(STDIN_FILENO, &c, 1)) {
	if (c == delim) {
	  try {
	    // ahhh c++, you never cease to disappoint. Appearantly declare j
	    // outside of the try leads to horrible random jumps. Why?
	    json j;
	    ss >> j;
	    RPCRequest req = parse_request(j);
	    std::cout << handle_request(&mic,req) << delim;
	  } catch (nlohmann::detail::parse_error &ex) {
	    std::cout << make_error(-32700, "Parse error") << std::endl;
	  } catch (json err) {
	    std::cout << err << delim;
	  }
	  ss.str("");
        } else {
          ss << c;
        }
      }
    }
  }
  mic.stop();
  return 0;
}
