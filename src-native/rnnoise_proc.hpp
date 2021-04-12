#pragma once

#include <bits/c++config.h>
#include <type_traits>
#include <vector>

#include <rnnoise.h>

#include "audio_processor.hpp"

class RNNoiseDenoiser : public AudioProcessor {
public:
  RNNoiseDenoiser();
  ~RNNoiseDenoiser();
  void feed(uint8_t *in, size_t size) override;
  size_t willspew() override;
  size_t spew(uint8_t *out, size_t maxsize) override;
  size_t get_buffer_size() override;
  void drop_samples(size_t size) override;
  void set_should_denoise(bool b) override { should_denoise = b; };
  size_t get_min_spew() override { return rnnoise_get_frame_size() * 2; };
  AudioFormat get_audio_format() { return AudioFormat::S16_LE; };
  int get_sample_rate() { return 48000; };

private:
  bool should_denoise = false;
  std::vector<uint8_t> in;
  DenoiseState *st;
};
static_assert(!std::is_abstract<RNNoiseDenoiser>::value,
              "RNNDenoiser must be concrete");

extern "C" {
AudioProcessor *create() { return new RNNoiseDenoiser(); }
void destroy(AudioProcessor *ap) { delete (RNNoiseDenoiser *)ap; }
}
static_assert(std::is_same<create_audio_processor_t, decltype(create)>::value,
              "Create is create_audio_processor_t");
static_assert(std::is_same<destroy_audio_processor_t, decltype(destroy)>::value,
              "Destroy is destroy_audio_processor_t");
