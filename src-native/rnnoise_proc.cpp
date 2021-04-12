#include "rnnoise_proc.hpp"
#include <iostream>

RNNoiseDenoiser::RNNoiseDenoiser() { st = rnnoise_create(NULL); }
RNNoiseDenoiser::~RNNoiseDenoiser() { rnnoise_destroy(st); }
void RNNoiseDenoiser::feed(uint8_t *arr, size_t size) {
  in.insert<uint8_t *>(in.end(), arr, arr + size);
}
size_t RNNoiseDenoiser::willspew() {
  if (!should_denoise) {
    return in.size();
  }
  return (in.size() / get_min_spew()) * get_min_spew();
}
size_t RNNoiseDenoiser::spew(uint8_t *out, size_t maxsize) {
  int i = 0;
  if (!should_denoise) {
    if (maxsize > in.size()) {
      maxsize = in.size();
    }
    std::copy(in.begin(), in.begin() + maxsize, out);
    i = maxsize;
  } else {
    float *temp = new float[rnnoise_get_frame_size()];

    for (i = 0; i <= (ssize_t)in.size() - (ssize_t)get_min_spew() &&
                i + get_min_spew() <= maxsize;
         i += get_min_spew()) {
      for (int j = 0; j < rnnoise_get_frame_size(); j++) {
        temp[j] = ((short *)in.data())[j];
      }
      rnnoise_process_frame(st, temp, temp);
      for (int j = 0; j < rnnoise_get_frame_size(); j++) {
        ((short *)out)[j] = temp[j];
      }
    }
    delete[] temp;
  }
  if (i) {
    in.erase(in.begin(), in.begin() + i);
  }
  return i;
}
size_t RNNoiseDenoiser::get_buffer_size() { return in.size(); }
void RNNoiseDenoiser::drop_samples(size_t samples) {
  if (samples >= in.size()) {
    samples = in.size();
  }
  in.erase(in.begin(), in.begin() + samples);
}
