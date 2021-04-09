#pragma once

#include <cstddef>

class AudioProcessor {
public:
  // Takes an array of floats, and copies them to internal storage
  virtual void feed(float *in, size_t size) = 0;
  // Returns how much the model could spew if requested right now
  virtual size_t willspew() = 0;
  // Spews up to maxsize floats. For now will spew a multiple of valid_length
  // less than or equal to maxsize so maxsize must be greater than min_spew
  virtual size_t spew(float *out, size_t maxsize) = 0;
  virtual size_t get_buffer_size() = 0;
  virtual void drop_samples(size_t size) = 0;
  virtual void set_should_denoise(bool b) = 0;
  virtual size_t get_min_spew() = 0;
};

typedef AudioProcessor *create_audio_processor_t();
typedef void destroy_audio_processor_t(AudioProcessor *);
