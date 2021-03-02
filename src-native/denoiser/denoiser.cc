#include <cassert>

#include "denoiser.h"

Denoiser::Denoiser(std::string ts_path) {
  options = torch::TensorOptions()
		     .dtype(torch::kFloat32)
    //.layout(torch::kStrided)
    //.device(torch::kCPU, 0)
                     .requires_grad(false);
  try {
    // Deserialize the ScriptModule from a file using torch::jit::load().
    module = torch::jit::load(ts_path);
  }
  catch (const c10::Error& e) {
    throw std::runtime_error(e.msg());
  }
  module.eval();
  lstm_hidden = torch::zeros(c10::IntArrayRef({2, 1, 384}), options);
  conv_hidden = c10::List<torch::Tensor>(
      {torch::zeros(c10::IntArrayRef({1, 48, 148}), options),
       torch::zeros(c10::IntArrayRef({1, 96, 36}), options),
       torch::zeros(c10::IntArrayRef({1, 192, 8}), options),
       torch::zeros(c10::IntArrayRef({1, 192, 4}), options),
       torch::zeros(c10::IntArrayRef({1, 96, 4}), options),
       torch::zeros(c10::IntArrayRef({1, 48, 4}), options),
       torch::zeros(c10::IntArrayRef({1, 1, 4}), options)});
}
void Denoiser::feed(float *arr, std::size_t size) {
  in.insert<float*>(in.end(), arr, arr+size);
  std.feed(arr, size);
}
std::size_t Denoiser::willspew() {
  if (!should_denoise) {
    return in.size();
  }
  // I can't figure out a good way to calculate this
  int written = 0;
  while (in.size() - written >= valid_length) {
    written += hop_size;
  }
  return written;
}
std::size_t Denoiser::spew(float *out, std::size_t maxsize) {
  int written = 0;
  if (!should_denoise) {
    written = maxsize;
    if (written > in.size()) {
      written = in.size();
    }
    std::copy(in.data(), in.data() + written, out);
  } else {
    while (written + hop_size <= maxsize &&
	   (in.size() - written) >= valid_length) {
      torch::Tensor audio =
	  torch::from_blob((float *)in.data() + written, valid_length, options);
      audio /= std.std();
      audio = audio.view(c10::IntArrayRef({1, 1, valid_length}));
      torch::jit::IValue model_out = module.forward(std::vector<torch::IValue>(
	  {torch::jit::IValue(audio), torch::jit::IValue(conv_hidden),
	   torch::jit::IValue(lstm_hidden)}));

      assert(model_out.isTuple());
      auto tuple = model_out.toTuple();
      auto clean = tuple->elements()[0].toTensor();
      clean *= std.std();
      conv_hidden = tuple->elements()[1].toTensorList();
      lstm_hidden = tuple->elements()[2].toTensor();

      clean = clean.contiguous();
      assert(clean.numel() == hop_size);
      std::copy(clean.data_ptr<float>(), clean.data_ptr<float>() + hop_size,
                out + written);
      written += hop_size;
    }
  }
  if (written) {
    in.erase(in.begin(), in.begin() + written);
  }
  return written;
}
