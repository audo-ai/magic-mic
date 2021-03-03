#pragma once
#include <optional>

#include <nlohmann/json.hpp>

using json = nlohmann::json;
using std::optional;
using std::string;

json make_error(int code, string message, optional<json> data = std::nullopt,
		optional<string> id = std::nullopt);
json make_response(string id, json result);
enum RequestTypes { GetStatus, GetMicrophones, SetMicrophone, SetRemoveNoise, SetLoopback };

struct RPCRequest {
  enum RequestTypes type;
  string id;
  union {
    struct {
      int mic_id;
    };
    struct {
      bool should_remove_noise;
    };
    struct {
      bool should_loopback;
    };
  };
};

RPCRequest parse_request(json j);
