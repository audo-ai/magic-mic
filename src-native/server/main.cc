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
#include <string.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "tray.h"

#include "virtual_mic.h"

#ifdef USE_PIPESOURCE
#include "pipesource_virtual_mic.h"
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

void handle_sigint(int sig) {
  spdlog::get("server")->info("Signal Received");
  running = false;
}
void write_json(int fd, json j) {
  string str = j.dump();
  str.push_back('\n');
  const char *c_str = str.c_str();
  int length = 0;
  while (length != str.length()) {
    ssize_t s = write(fd, c_str + length, str.length() - length);
    if (s == -1) {
      spdlog::get("server")->debug("Failed to write out response");
      return;
    }
    length += s;
  }
}

void quit_tray_cb(struct tray_menu *menu) {
  *(bool*)menu->context = true;
}
void open_tray_cb(struct tray_menu *menu) {
  *(bool*)menu->context = true;
}
int main(int argc, char **argv) {
  auto logger = spdlog::stderr_color_mt("server");
  spdlog::set_level(spdlog::level::trace);

  if (argc != 4) {
    logger->error("USAGE: {} SOCK_PATH ICON_PATH APP_PATH", argv[0]);
    return 1;
  }
  char *sock_path = argv[1];
  char *icon_path = argv[2];
  char *app_path = argv[3];

  unlink(sock_path);
  
  int sock_fd = -1;
  int serv_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (!serv_fd) {
    logger->error("socket(AF_UNIX, SOCK_STREAM, 0): {}", strerror(errno));
    return 1;
  }
  struct sockaddr_un addr;
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strcpy(addr.sun_path, sock_path);

  if (-1 == bind(serv_fd, (sockaddr *)&addr, sizeof(addr))) {
    logger->error("bind(...): {}", strerror(errno));
    return 1;
  }
  if (-1 == listen(serv_fd, 0)) {
    logger->error("listen(...): {}", strerror(errno));
    return 1;
  }
  fcntl(serv_fd, F_SETFL, O_NONBLOCK);

  signal(SIGINT, handle_sigint);
  signal(SIGHUP, SIG_IGN);

  bool tray_exit_requested = false;
  bool open_app_requested = false;
  // The warnings for casting string const to char* were annoying
  char quit_text[] = "Quit";
  char open_text[] = "Open";
  struct tray_menu menu[] = {{quit_text, 0, 0, quit_tray_cb, &tray_exit_requested},
			     {open_text, 0, 0, open_tray_cb, &open_app_requested},
			     {nullptr, 0, 0, nullptr, nullptr}};
  struct tray tray = {
      .icon = icon_path,
      .menu = (struct tray_menu *)&menu,
  };
  if (tray_init(&tray) < 0) {
    logger->error("Unable to create systray item");
    return 1;
  }

  logger->info("Starting {} virtual mic", VIRTUAL_MIC_NAME);
  auto l = spdlog::stderr_color_mt("virtmic");
  l->set_level(spdlog::level::trace);
  ConcreteVirtualMic mic(l);
  auto err_fut = mic.get_exception_future();

  RPCServer server(&mic);

  stringstream ss;
  while (running) {
    if (std::future_status::ready == err_fut.wait_for(std::chrono::milliseconds(0))) {
      try {
	std::rethrow_exception(err_fut.get());
      } catch (const std::exception &e) {
	logger->error("Virtual Mic through exception \"{}\"", e.what());
	break;
      }
    }

    fd_set rfds;
    struct timespec tv;
    sigset_t mask;
    int ret;
    FD_ZERO(&rfds);
    FD_SET(serv_fd, &rfds);
    if (-1 != sock_fd) {
      FD_SET(sock_fd, &rfds);
    }
    tv.tv_sec = 0;
    // 10 mil nano seconds = 10 milis?
    // 100mil would probably be fine, but it tray doesn't seem to work at 100millis
    tv.tv_nsec = 10000000;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);

    // serv_fd is necessarily created first so will be smaller
    ret = pselect(1 + (sock_fd == -1 ? serv_fd : sock_fd), &rfds, nullptr, nullptr, &tv, &mask);
    tray_loop(0);
    if (tray_exit_requested) {
      logger->info("tray quit called");
      if (sock_fd == -1) {
	logger->info("No current connections; exiting");
	running = false;
      } else {
	logger->info("Cilent connected, not exiting yet");
	tray_exit_requested = false;
	menu[0].checked = 0;
	menu[1].checked = 1;
	tray_update(&tray);
      }
    }

    if (open_app_requested) {
      open_app_requested = false;
      if (sock_fd == -1) {
	logger->info("Starting app");
	if (0 == fork()) {
	  // child
	  execl(app_path, nullptr);
	} else {
	  // parent
	}
      } else {
	logger->error("Attempted to open app with connection active");
      }
    }

    if (ret == -1) {
      logger->error("pselect(...): {}", strerror(errno));
      running = false;
    }

    if (FD_ISSET(serv_fd, &rfds)) {
      if (sock_fd != -1) {
	logger->debug("Attempt to connect to socket while already associated");
	if (-1 == (sock_fd = accept(serv_fd, nullptr, nullptr))) {
	  logger->debug("Error accepting connection (even though we will imediately close): {}", strerror(errno));
	} else {
	  close(sock_fd);
	}
	sock_fd = -1;
      } else {
	logger->info("Accepting connection");
	if (-1 == (sock_fd = accept(serv_fd, nullptr, nullptr))) {
	  logger->error("Error accepting conection: {}", strerror(errno));
	  running = false;
	} else {
	  fcntl(sock_fd, F_SETFL, O_NONBLOCK);
	  menu[0].disabled = 1;
	  menu[1].disabled = 1;
	  menu[1].checked = 1;
	  tray_update(&tray);
	}
      }
    }

    if (sock_fd != -1 && FD_ISSET(sock_fd, &rfds)) {
      char c;
      int r;
      int total_read = 0;
      while (1 == (r = read(sock_fd, &c, 1))) {
	total_read += r;
	if (c == delim) {
	  try {
	    // ahhh c++, you never cease to disappoint. Appearantly declare j
	    // outside of the try leads to horrible random jumps. Why?
	    json j;
	    ss >> j;
	    server.handle_request(j);
	  } catch (nlohmann::detail::parse_error &ex) {
	    write_json(sock_fd, server.make_error(-32700, "Parse error"));
	  } catch (json err) {
	    write_json(sock_fd, err);
	  }
	  ss.str("");
        } else {
          ss << c;
        }
      }
      if (r == 0) {
	logger->debug("Connection to socket closed");
	sock_fd = -1;
	menu[0].disabled = 0;
	menu[1].disabled = 0;
	menu[1].checked = 0;
	tray_update(&tray);

	server.clear();
      }
    }
    if (sock_fd != -1) {
      server.pump();
      vector<json> to_write = server.pop_responses();
      for (auto j : to_write) {
	write_json(sock_fd, j);
      }
    }
  }
  mic.stop();
  return 0;
}

