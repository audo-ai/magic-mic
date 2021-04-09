#pragma once

#include <future>
#include <optional>
#include <string>
#include <utility>
#include <vector>

using std::future;
using std::pair;
using std::string;
using std::vector;
using std::optional;

struct VirtualMicUpdate {
  enum { UpdateAudioProcessing } update;
  union {
    bool audioProcessingValue;
  };
};
/*
  VirtualMic's are expected to start their own thread and handle these functions
  concurrently
 */
class VirtualMic {
public:
  virtual void stop() = 0;
  virtual void abortLastRequest() = 0;
  virtual future<bool> getStatus() = 0;
  virtual future<pair<int, vector<pair<int, string>>>> getMicrophones() = 0;
  virtual future<void> setMicrophone(int) = 0;
  virtual future<void> setRemoveNoise(bool) = 0;
  virtual future<bool> setLoopback(bool) = 0;

  virtual future<std::exception_ptr> get_exception_future() = 0;
  virtual optional<VirtualMicUpdate> get_update() = 0;
};
