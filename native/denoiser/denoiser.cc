
// This is included here to hide it from the puleaudio stuff. They don't place nicely
#include <torch/script.h>

#include "denoiser.h"

Denoiser::Denoiser() {

}
// TODO since pulseaudio can't touch pytorch we need to construct tensors here,
// which means we either have to copy, or take a deleter. I think taking a
// deleter would be better
void Denoiser::feed(float *arr, std::size_t size) {
  in.insert<float*>(in.end(), arr, arr+size);
}
std::size_t Denoiser::willspew() {
  return in.size();
}
std::size_t Denoiser::spew(float *out, std::size_t maxsize) {
  torch::Tensor temp = torch::from_blob((float*)in.data(), in.size());
  temp = temp.contiguous();
  std::copy(temp.data_ptr<float>(), temp.data_ptr<float>()+in.size(), out);
  in.resize(0);
  return in.size();
}
