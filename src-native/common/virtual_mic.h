#pragma once

#include <string>
#include <vector>
#include <utility>
#include <future>

using std::future;
using std::pair;
using std::string;
using std::vector;
/*
  VirtualMic's are expected to start their own thread and handle these functions
  concurrently
 */
class VirtualMic {
public:
  virtual void stop() = 0;
  virtual void abortLastRequest() = 0;
  virtual future<bool> getStatus() = 0;
  virtual future<vector<pair<int, string>>> getMicrophones() = 0;
  virtual future<void> setMicrophone(int) = 0;
  virtual future<void> setRemoveNoise(bool) = 0;

  virtual future<std::exception_ptr> get_exception_future() = 0;
};
