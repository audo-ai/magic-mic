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
  Info get_info() override;
  void process(uint8_t *in, uint8_t *out) override;
  size_t get_chunk_size() override {
    return rnnoise_get_frame_size() * sizeof(short);
  };
  AudioFormat get_audio_format() override { return AudioFormat::S16_LE; };
  int get_sample_rate() override { return 48000; };

private:
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
