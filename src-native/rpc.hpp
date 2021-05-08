#pragma once
#include <future>
#include <map>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

#include "nlohmann/json.hpp"

#include "audio_processor_manager.hpp"
#include "virtual_mic.hpp"

using json = nlohmann::json;
using std::future;
using std::map;
using std::optional;
using std::pair;
using std::shared_ptr;
using std::string;
using std::variant;
using std::vector;

class RPCServer {
public:
  RPCServer(VirtualMic *mic, AudioProcessorManager apm);
  void handle_request(json req);
  vector<json> pop_responses();
  void pump();
  void clear();

  static json make_error(int code, string message,
                         optional<json> data = std::nullopt,
                         optional<string> id = std::nullopt);
  static json make_response(string id, json result);

private:
  enum RequestTypes {
    GetStatus,
    GetMicrophones,
    SetMicrophone,
    SetRemoveNoise,
    GetRemoveNoise,
    SetLoopback,
    GetLoopback,
    GetProcessors,
    SetProcessor
  };

  struct Request {
    enum RequestTypes type;
    string id;
    union {
      struct {
        // SetMicrophone
        int micId;
      };
      struct {
        // SetRemoveNoise
        bool shouldRemoveNoise;
      };
      struct {
        // SetLoopback
        bool shouldLoopback;
      };
      struct {
	// setProcessor
	int apId;
      };
    };
  };

  struct Response {
    enum RequestTypes type;
    string id;
    variant<future<bool>, future<pair<int, vector<pair<int, string>>>>,
            future<void>, future<pair<int, vector<AudioProcessor::Info>>>>
        fut;
  };
  VirtualMic *mic;
  AudioProcessorManager apm;
  vector<Request> request_queue;
  optional<Response> current_response;

  Request parse_request(json j);
};
