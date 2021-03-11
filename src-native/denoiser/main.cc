#include <iostream>
#include <vector>
#include <fstream>
#include "denoiser.h"
using namespace std;
int main(int argc, char **argv) {
  ifstream file_in;
  ofstream file_out;
  file_in.open(argv[1], ios::binary | ios::in);
  vector<float> in;
  float data;
  while (file_in.read((char*)&data, sizeof(float)).good()) {
    in.push_back(data);
  }
  std::cout << "read: "<<in.size() << std::endl;
  Denoiser denoiser("/home/gabe/code/audo/project-x/src-tauri/models/audo-realtime-denoiser.v1.ts");
  denoiser.feed(&*in.begin(), in.size());
  float *buf = new float[in.size()];
  denoiser.should_denoise =true;
  auto spewed = denoiser.spew(buf, in.size());
  file_out.open(argv[2], ios::binary | ios::out | ios::trunc);
  file_out.write((char*)buf, sizeof(float)*spewed);
}
