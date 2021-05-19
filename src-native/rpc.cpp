#include "rpc.hpp"

json RPCServer::make_error(int code, string message, optional<json> data,
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
json RPCServer::make_response(string id, json result) {
  json out;
  out["jsonrpc"] = "2.0";
  out["id"] = id;
  out["result"] = result;
  return out;
}

RPCServer::Request RPCServer::parse_request(json j) {
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
  Request req;
  if (!j["id"].is_string()) {
    throw make_error(-32603, "Internal Error",
                     json("We do not support non string id"));
  }
  req.id = j["id"].get<std::string>();
  std::string method = j["method"].get<std::string>();
  if (method == "getStatus") {
    req.type = RequestTypes::GetStatus;
  } else if (method == "getMicrophones") {
    req.type = RequestTypes::GetMicrophones;
  } else if (method == "setMicrophone") {
    if (!j["params"].is_number()) {
      throw make_error(-32602, "Invalid Params",
                       "param must be number for setMicrophone", req.id);
    }
    req.type = RequestTypes::SetMicrophone;
    req.micId = j["params"].get<int>();
  } else if (method == "setRemoveNoise") {
    if (!j["params"].is_boolean()) {
      throw make_error(-32602, "Invalid Params",
                       "param must be bool for setRemoveNoise", req.id);
    }
    req.type = RequestTypes::SetRemoveNoise;
    req.shouldRemoveNoise = j["params"].get<bool>();
  } else if (method == "setLoopback") {
    if (!j["params"].is_boolean()) {
      throw make_error(-32602, "Invalid Params",
                       "param must be bool for setLoopback", req.id);
    }
    req.type = RequestTypes::SetLoopback;
    req.shouldLoopback = j["params"].get<bool>();
  } else if (method == "getLoopback") {
    req.type = RequestTypes::GetLoopback;
  } else if (method == "getRemoveNoise") {
    req.type = RequestTypes::GetRemoveNoise;
  } else if (method == "getProcessors") {
    req.type = RequestTypes::GetProcessors;
  } else if (method == "setProcessor") {
    req.type = RequestTypes::SetProcessor;
    req.apId = j["params"].get<int>();
  } else {
    throw make_error(-32600, "Invalid Request");
  }
  return req;
}

void RPCServer::handle_request(json j) {
  Request req = parse_request(j);

  request_queue.push_back(req);
  pump();
}
void RPCServer::clear() {
  current_response.reset();
  request_queue = vector<Request>();
}
void RPCServer::pump() {
  if (!current_response && !request_queue.empty()) {
    Request req = request_queue.front();
    request_queue.erase(request_queue.begin());

    switch (req.type) {
    case RequestTypes::GetStatus: {
      current_response = {
          .type = req.type, .id = req.id, .fut = mic->getStatus()};
      break;
    }
    case RequestTypes::GetMicrophones: {
      current_response = {
          .type = req.type, .id = req.id, .fut = mic->getMicrophones()};
      break;
    }
    case RequestTypes::SetMicrophone: {
      current_response = {
          .type = req.type, .id = req.id, .fut = mic->setMicrophone(req.micId)};
      break;
    }
    case RequestTypes::SetRemoveNoise: {
      current_response = {.type = req.type,
                          .id = req.id,
                          .fut = mic->setRemoveNoise(req.shouldRemoveNoise)};
      break;
    }
    case RequestTypes::GetRemoveNoise: {
      current_response = {
          .type = req.type, .id = req.id, .fut = mic->getRemoveNoise()};
      break;
    }
    case RequestTypes::SetLoopback: {
      current_response = {.type = req.type,
                          .id = req.id,
                          .fut = mic->setLoopback(req.shouldLoopback)};
      break;
    }
    case RequestTypes::GetLoopback: {
      current_response = {
          .type = req.type, .id = req.id, .fut = mic->getLoopback()};
      break;
    }
    case RequestTypes::GetProcessors: {
      // TODO: do we really want to fulfill request in rpc?
      std::promise<pair<int, vector<AudioProcessor::Info>>> p;
      current_response = {
          .type = req.type, .id = req.id, .fut = p.get_future()};
      vector<AudioProcessor::Info> infos;
      for (auto &ap : apm.get_audio_processors()) {
        infos.push_back(ap.get()->get_info());
      }
      p.set_value(std::make_pair(apm.get_current(), infos));
      break;
    }
    case RequestTypes::SetProcessor: {
      std::promise<void> p;
      // TODO Bounds checking and whatnot
      mic->setAudioProcessor(apm.set_current(req.apId).get());
      current_response = {
          .type = req.type, .id = req.id, .fut = p.get_future()};
      p.set_value();
      break;
    }
    }
  }
}
vector<json> RPCServer::pop_responses() {
  if (!current_response) {
    return vector<json>();
  }
  vector<json> out;
  Response &resp = current_response.value();

  switch (resp.type) {
  case RequestTypes::GetLoopback:
  case RequestTypes::GetRemoveNoise:
  case RequestTypes::SetLoopback:
  case RequestTypes::GetStatus: {
    auto &fut = std::get<future<bool>>(resp.fut);
    if (fut.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
      out.push_back(make_response(resp.id, fut.get()));
      current_response.reset();
    }
    break;
  }
  case RequestTypes::GetMicrophones: {
    auto &f = std::get<future<pair<int, vector<pair<int, string>>>>>(resp.fut);
    if (f.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
      break;
    }
    auto val = f.get();

    json list = json::basic_json::array({});
    for (auto &tuple : std::get<1>(val)) {
      list.push_back(
          {{"id", std::get<0>(tuple)}, {"name", std::get<1>(tuple)}});
    }
    json j;
    j["list"] = list;
    j["cur"] = std::get<0>(val);

    out.push_back(make_response(resp.id, j));
    current_response.reset();
    break;
  }
  case RequestTypes::SetProcessor:
  case RequestTypes::SetMicrophone: {
    auto &f = std::get<future<void>>(resp.fut);
    if (f.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
      out.push_back(make_response(resp.id, {}));
      current_response.reset();
    }
    break;
  }
  case RequestTypes::SetRemoveNoise: {
    auto &f = std::get<future<void>>(resp.fut);
    if (f.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
      out.push_back(make_response(resp.id, {}));
      current_response.reset();
    }
    break;
  }
  case RequestTypes::GetProcessors: {
    auto &f =
        std::get<future<pair<int, vector<AudioProcessor::Info>>>>(resp.fut);
    if (f.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
      auto v = f.get();
      vector<json> infos;
      for (auto &info : std::get<1>(v)) {
        infos.push_back({{"name", info.name}});
      }
      out.push_back(
          make_response(resp.id, {{"list", infos}, {"cur", std::get<0>(v)}}));
      current_response.reset();
    }
    break;
  }
  }
  return out;
}
RPCServer::RPCServer(VirtualMic *mic, AudioProcessorManager apm) : apm(apm) {
  this->mic = mic;
}
