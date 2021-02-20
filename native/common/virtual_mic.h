#pragma once

#include <string>
#include <vector>
#include <utility>
#include <future>

using std::promise;
using std::pair;
using std::string;
using std::vector;
/*
  VirtualMic's are expected to start their own thread and handle these functions
  concurrently
 */
class VirtualMic {
public:
  virtual void stop() = 0;
  virtual void getStatus(promise<bool>) = 0;
  virtual void getMicrophones(promise<vector<pair<int, string>>>) = 0;
  virtual void setMicrophone(promise<void>, int) = 0;
  virtual void setRemoveNoise(promise<void>, bool) = 0;
};
