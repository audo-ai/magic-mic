#include "rpc.h"

RPCRequest parse_request(json j) {
  json err;
  if (!j.is_object()) {
    throw make_error(-32600, "Invalid Request");
  }
  if (!(j["jsonrpc"].is_string() && j["jsonrpc"].get<std::string>() == "2.0")) {
    throw make_error(-32600, "Invalid Request");
  }
  if (!j["method"].is_string()) {
    throw make_error(-32600, "Invalid Request");
  }
  RPCRequest req;
  if (!j["id"].is_string()) {
    throw make_error(-32603, "Internal Error",
		     json("We do not support non string id"));
  }
  req.id = j["id"].get<std::string>();
  std::string method = j["method"].get<std::string>();
  if (method == "getStatus") {
    req.type = GetStatus;
  } else if (method == "getMicrophones") {
    req.type = GetMicrophones;
  } else if (method == "setMicrophone") {
    if (!j["params"].is_number()) {
      throw make_error(-32602, "Invalid Params",
		       "param must be number for setMicrophone", req.id);
    }
    req.type = SetMicrophone;
    req.mic_id = j["params"].get<int>();
  } else if (method == "setRemoveNoise") {
    if (!j["params"].is_boolean()) {
      throw make_error(-32602, "Invalid Params",
		       "param must be bool for setRemoveNoise", req.id);
    }
    req.type = SetRemoveNoise;
    req.mic_id = j["params"].get<bool>();
  }
  return req;
}
json make_error(int code, string message, optional<json> data,
		optional<string> id) {
  json out;
  out["jsonrpc"] = "2.0";
  out["error"] = {{"code", code}, {"message", message}};
  if (id) {
    out["id"] = id.value();
  } else {
    out["id"] = nullptr;
  }
  if (data) {
    out["error"]["data"] = data.value();
  }
  return out;
}
json make_response(string id, json result) {
  json out;
  out["jsonrpc"] = "2.0";
  out["id"] = id;
  out["result"] = result;
  return out;
}
