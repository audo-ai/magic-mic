#pragma once

#include <memory>
#include <vector>

#include "audio_processor.hpp"

using std::shared_ptr;
using std::vector;

class AudioProcessorManager {
public:
  AudioProcessorManager(string path);
  int get_current() { return current; }
  shared_ptr<AudioProcessor> set_current(int i) {
    current = i;
    return aps[i];
  };
  vector<shared_ptr<AudioProcessor>> get_audio_processors() { return aps; };

private:
  int current;
  vector<shared_ptr<AudioProcessor>> aps;
};
