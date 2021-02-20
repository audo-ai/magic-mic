#include <iostream>
#include <thread>
#include <csignal>
#include <sstream>
#include <optional>

#include <sys/select.h>
#include <errno.h>
#include <unistd.h>

#include <nlohmann/json.hpp>


#include <virtual_mic.h>

#ifdef USE_PIPESOURCE
#include <pipesource_virtual_mic.h>
typedef PipeSourceVirtualMic ConcreteVirtualMic;
#define VIRTUAL_MIC_NAME "PIPESOURCE"
#endif

using json = nlohmann::json;
using std::stringstream;
using std::optional;

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
json make_error(int code, std::string message, optional<json> data = std::nullopt,
		optional<std::string> id = std::nullopt) {
  json out;
  out["jsonrpc"] = "2.0";
  out["error"] = {{"code", code}, {"message", message}};
  if (id) {
    out["id"] = id.value();
  } else {
    out["id"] = nullptr;
  }
  if (data) {
    out["data"] = data.value();
  }
  return out;
}
enum RequestTypes { GetStatus, GetMicrophones, SetMicrophone, SetRemoveNoise };
struct RPCRequest {
  enum RequestTypes type;
  std::string id;
  union {
    struct { int mic_id; };
    struct { bool should_remove_noise; };
  };
};
RPCRequest parse_request(json j) {
  json err;
  if (!(j["jsonrpc"].is_string() && j["jsonrpc"].get<std::string>() == "2.0")) {
    throw make_error(-32600, "Invalid Request");
  }
  if (!j["method"].is_string()) {
    throw make_error(-32600, "Invalid Request");
  }
  RPCRequest req;
  if (!j["id"].is_string()) {
    throw make_error(-32603, "Internal Error", json("We do not support non string id"));
  }
  req.id = j["id"].get<std::string>();
  std::string method = j["method"].get<std::string>();
  if (method == "getStatus") {
    req.type = GetStatus;
  } else if (method == "getMicrophones") {
    req.type = GetMicrophones;
  } else if (method == "setMicrophone") {
    if (!j["params"].is_number()) {
      throw make_error(-32602, "Invalid Params", "param must be number for setMicrophone", req.id);
    }
    req.type = SetMicrophone;
    req.mic_id = j["params"].get<int>();
  } else if (method == "setRemoveNoise") {
    if (!j["params"].is_boolean()) {
      throw make_error(-32602, "Invalid Params", "param must be bool for setRemoveNoise", req.id);
    }
    req.type = SetRemoveNoise;
    req.mic_id = j["params"].get<bool>();
  }
  return req;
}

constexpr char delim = '\n';
int main () {
  signal(SIGINT, handle_signal);
  std::cout << "Starting " << VIRTUAL_MIC_NAME << " virtual mic" << std::endl;
  //ConcreteVirtualMic mic;


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
      auto read_in = read(STDIN_FILENO, line, sizeof(line));
      if (read_in > 0 && (line[read_in - 1] == delim)) {
	json j;
	ss << line;
        try {
          ss >> j;
	  RPCRequest req = parse_request(j);
	  std::cerr << "Handling: " << req.type << std::endl;
        } catch (nlohmann::detail::parse_error& ex) {
	  std::cout << make_error(-32700, "Parse error") << std::endl;
	} catch (json err) {
	  std::cout << err;
	}
        ss.str("");
      } else if (read_in > 0) {
        ss << line;
      }
    }
  }
  //mic.stop();
  return 0;
}
