#pragma once

#include <cstddef>

// TODO maybe template this
class RunningSTD {
 public:
  RunningSTD();
  void feed(float *arr, size_t size);
  float std();
};
