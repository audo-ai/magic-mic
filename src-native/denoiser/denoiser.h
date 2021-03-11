#pragma once

#include <vector>
#include <bits/c++config.h>

#include <torch/script.h>

#include "running_std.h"

// TODO make this  
class Denoiser {
 public:
  Denoiser(std::string ts_path);
  // Takes an array of floats, and copies them to internal storage
  void feed(float *in, std::size_t size);
  // Returns how much the model could spew if requested right now
  std::size_t willspew();
  // Spews up to maxsize floats. For now will spew a multiple of valid_length
  // less than or equal to maxsize so maxsize must be greater than min_spew
  std::size_t spew(float *out, std::size_t maxsize);
  std::size_t get_size_multiple();

  bool should_denoise = false;

  const int min_spew = hop_size;

 private:
  int valid_length = 596;
  int hop_size = 256;

  at::TensorOptions options;

  RunningSTD std;
  torch::jit::script::Module module;
  torch::Tensor lstm_hidden;
  c10::List<torch::Tensor> conv_hidden;
  std::vector<float> in;
};
