#include "rnnoise_proc.hpp"
#include <iostream>

AudioProcessor::Info RNNoiseDenoiser::get_info() {
  return {.name = "lightweight denoiser (rnnoise)"};
}
RNNoiseDenoiser::RNNoiseDenoiser() { st = rnnoise_create(NULL); }
RNNoiseDenoiser::~RNNoiseDenoiser() { rnnoise_destroy(st); }
void RNNoiseDenoiser::process(uint8_t *in, uint8_t *out) {
  float *temp = new float[rnnoise_get_frame_size()];

  for (int j = 0; j < rnnoise_get_frame_size(); j++) {
    temp[j] = ((short *)in)[j];
  }
  rnnoise_process_frame(st, temp, temp);
  for (int j = 0; j < rnnoise_get_frame_size(); j++) {
    ((short *)out)[j] = temp[j];
  }
}
