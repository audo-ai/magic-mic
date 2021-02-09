#pragma once

#include <vector>

#include <bits/c++config.h>

// TODO make this  
class Denoiser {
 public:
  Denoiser();
  // Takes an array of floats, and copies them to internal storage
  void feed(float *in, std::size_t size);
  // Maybe returns some amount of processed audio
  std::size_t willspew();
  std::size_t spew(float *out, std::size_t maxsize);
  std::size_t get_size_multiple();
 public:
  std::vector<float> in;
};
