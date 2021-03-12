#include <cassert>

#include "denoiser.h"

Denoiser::Denoiser(std::string ts_path) : in(valid_length, 0.0) {
  options = torch::TensorOptions()
    .dtype(torch::kFloat32)
    .layout(torch::kStrided)
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
void Denoiser::feed(float *arr, size_t size) {
  in.insert<float*>(in.end(), arr, arr+size);
  std.feed(arr, size);
}
size_t Denoiser::willspew() {
  if (!should_denoise) {
    return in.size();
  }
  // Well, that was actually pretty simple
  if (in.size() < valid_length) {
    return 0;
  }
  ssize_t would_spew = (hop_size)*((in.size() - valid_length)/hop_size + 1);
  return would_spew > 0 ? would_spew : 0;
}
size_t Denoiser::spew(float *out, size_t maxsize) {
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

      torch::NoGradGuard no_grad;

      torch::Tensor audio(torch::from_blob((float *)in.data() + written, valid_length, options));
      audio.slice(0, valid_length - hop_size) /= std.std();
      audio = audio.view(c10::IntArrayRef({1, 1, valid_length}));
      torch::jit::IValue model_out = module.forward(std::vector<torch::IValue>(
	  {torch::jit::IValue(audio), torch::jit::IValue(conv_hidden),
	   torch::jit::IValue(lstm_hidden)}));

      assert(model_out.isTuple());
      auto tuple = model_out.toTuple();
      auto clean = tuple->elements()[0].toTensor();
      conv_hidden = tuple->elements()[1].toTensorList();
      lstm_hidden = tuple->elements()[2].toTensor();

      clean *= std.std();
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
size_t Denoiser::get_buffer_size() {
  return in.size();
}
void Denoiser::drop_samples(size_t samples) {
  if (samples >= in.size()) {
    samples = in.size();
  }
  in.erase(in.begin(), in.begin() + samples);
}
