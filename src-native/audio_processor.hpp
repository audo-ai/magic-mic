#pragma once

#include <bits/stdint-uintn.h>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

using std::optional;
using std::shared_ptr;
using std::string;
using std::vector;

enum AudioFormat {
  FLOAT32_LE,
  S16_LE,
};

class AudioProcessor {
public:
  struct Info {
    string name;
  };
  // Takes an array of floats, and copies them to internal storage
  virtual Info get_info() = 0;
  // in and out must be of get_chunk_size
  virtual void process(uint8_t *in, uint8_t *out) = 0;
  // in bytes
  virtual size_t get_chunk_size() = 0;
  virtual AudioFormat get_audio_format() = 0;
  virtual int get_sample_rate() = 0;
  virtual optional<string> get_lower_load_instructions() {
    return std::nullopt;
  };
};

typedef AudioProcessor *create_audio_processor_t();
typedef void destroy_audio_processor_t(AudioProcessor *);

vector<shared_ptr<AudioProcessor>> load_audioprocs(string dir);
