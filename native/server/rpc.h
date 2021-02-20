#pragma once
#include <optional>

#include <nlohmann/json.hpp>

using json = nlohmann::json;
using std::optional;
using std::string;

json make_error(int code, string message, optional<json> data = std::nullopt,
		optional<string> id = std::nullopt);
enum RequestTypes { GetStatus, GetMicrophones, SetMicrophone, SetRemoveNoise };

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
  };
};

RPCRequest parse_request(json j);
