#pragma once

#include <future>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "audio_processor.hpp"

using std::future;
using std::optional;
using std::pair;
using std::string;
using std::vector;

struct VirtualMicUpdate {
  string text;
  bool notify;
};
/*
  VirtualMic's are expected to start their own thread and handle these functions
  concurrently
 */
class VirtualMic {
public:
  VirtualMic(AudioProcessor *ap) : ap(ap){};
  virtual ~VirtualMic(){};
  virtual void stop() = 0;
  virtual void abortLastRequest() = 0;
  virtual future<bool> getStatus() = 0;
  virtual future<pair<int, vector<pair<int, string>>>> getMicrophones() = 0;
  virtual future<void> setMicrophone(int) = 0;
  virtual future<void> setRemoveNoise(bool) = 0;
  virtual future<bool> getRemoveNoise() = 0;
  virtual future<bool> setLoopback(bool) = 0;
  virtual future<bool> getLoopback() = 0;
  virtual void setAudioProcessor(AudioProcessor *ap) = 0;

  virtual future<std::exception_ptr> get_exception_future() = 0;
  virtual optional<VirtualMicUpdate> get_update() = 0;

protected:
  AudioProcessor *ap;
};
