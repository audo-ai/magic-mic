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
#include <sys/socket.h>
#include <sys/un.h>
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
  // TODO: DRY
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
    try {
      wait_on_future<void>(f);
    } catch(interrupted_error &e) {
      return make_error(-32000, "Server error", "Interrupted while waiting for response");
    }
    return make_response(req.id, {});
  }
  case SetRemoveNoise: {
    future<void> f = mic->setRemoveNoise(req.should_remove_noise);
    try {
      wait_on_future<void>(f);
    } catch(interrupted_error &e) {
      return make_error(-32000, "Server error", "Interrupted while waiting for response");
    }
    return make_response(req.id, {});
  }
  case SetLoopback: {
    future<bool> f = mic->setLoopback(req.should_loopback);
    try {
      wait_on_future<bool>(f);
    } catch(interrupted_error &e) {
      return make_error(-32000, "Server error", "Interrupted while waiting for response");
    }
    return make_response(req.id, f.get());
  }
  default:
    throw std::runtime_error("Unknown request enountered in handle_request");
  }
}
void write_json(int fd, json j) {
  string str = j.dump();
  str.push_back('\n');
  const char *c_str = str.c_str();
  int length = 0;
  while (length != str.length()) {
    ssize_t s = write(fd, c_str + length, str.length() - length);
    if (s == -1) {
      throw std::runtime_error(strerror(errno));
    }
    length += s;
  }
}
int main(int argc, char **argv) {
  if (argc != 3) {
    std::cerr << "USAGE: " << argv[0] << " SOCK_PATH MODEL_PATH" << std::endl;
    return 1;
  }

  int sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (!sock_fd) {
    perror("socket");
    return 1;
  }
  struct sockaddr_un addr;
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strcpy(addr.sun_path, argv[1]);

  if (-1 == connect(sock_fd, (sockaddr *)&addr, sizeof(addr))) {
    perror("connect");
    return 1;
  }
  fcntl(sock_fd, F_SETFL, O_NONBLOCK);

  signal(SIGINT, handle_signal);
  std::cerr << "Starting " << VIRTUAL_MIC_NAME << " virtual mic" << std::endl;
  ConcreteVirtualMic mic(argv[2]);
  auto err_fut = mic.get_exception_future();

  stringstream ss;
  while (running) {
    if (std::future_status::ready == err_fut.wait_for(std::chrono::milliseconds(0))) {
      try {
	std::rethrow_exception(err_fut.get());
      } catch (const std::exception &e) {
	std::cerr << "Virtual Mic through exception: \"" << e.what() << "\"\n";
	break;
      }
    }

    fd_set rfds;
    struct timespec tv;
    sigset_t mask;
    int ret;
    FD_ZERO(&rfds);
    FD_SET(sock_fd, &rfds);
    tv.tv_sec = 1;
    tv.tv_nsec = 0;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);

    ret = pselect(sock_fd + 1, &rfds, nullptr, nullptr, &tv, &mask);
    if (ret == -1) {
      std::cerr << "pselect error: " << strerror(errno) << std::endl;
      running = false;
    }

    if (FD_ISSET(sock_fd, &rfds)) {
      char c;
      int r;
      while (1 == (r = read(sock_fd, &c, 1))) {
	if (c == delim) {
	  try {
	    // ahhh c++, you never cease to disappoint. Appearantly declare j
	    // outside of the try leads to horrible random jumps. Why?
	    json j;
	    ss >> j;
	    RPCRequest req = parse_request(j);
	    write_json(sock_fd, handle_request(&mic,req));
	  } catch (nlohmann::detail::parse_error &ex) {
	    write_json(sock_fd, make_error(-32700, "Parse error"));
	  } catch (json err) {
	    write_json(sock_fd, err);
	  }
	  ss.str("");
        } else {
          ss << c;
        }
      }
      if (r == 0) { break; }
    }
  }
  mic.stop();
  return 0;
}
