#include <cassert>

#include "denoiser.h"

Denoiser::Denoiser(std::string ts_path) {
  options = torch::TensorOptions()
		     .dtype(torch::kFloat32)
    //.layout(torch::kStrided)
    //.device(torch::kCPU, 0)
                     .requires_grad(false);
  torch::jit::script::Module module;
  try {
    // Deserialize the ScriptModule from a file using torch::jit::load().
    module = torch::jit::load(ts_path);
    module.dump(true, false, false);
  }
  catch (const c10::Error& e) {
    throw e.msg();
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
// TODO since pulseaudio can't touch pytorch we need to construct tensors here,
// which means we either have to copy, or take a deleter. I think taking a
// deleter would be better
void Denoiser::feed(float *arr, std::size_t size) {
  in.insert<float*>(in.end(), arr, arr+size);
  std.feed(arr, size);
}
std::size_t Denoiser::willspew() {
  if (in.size() >= valid_length) {
    return in.size() - (in.size() % valid_length);
  }
  return 0;
}
std::size_t Denoiser::spew(float *out, std::size_t maxsize) {
  int chunk;
  for (chunk = 1; chunk * valid_length <= maxsize && chunk*valid_length <= in.size(); chunk++) {
    torch::Tensor audio = torch::from_blob((float *)in.data() + (chunk-1)*valid_length, valid_length, options);
    std::cout<<audio.numel();
    audio /= std.std();
    audio = audio.view(c10::IntArrayRef({1,1,valid_length}));
    torch::jit::IValue model_out = module.forward(std::vector<torch::IValue>({torch::jit::IValue(audio), torch::jit::IValue(conv_hidden), torch::jit::IValue(lstm_hidden)}));

    assert(model_out.isTuple());
    auto tuple = model_out.toTuple();
    auto clean = tuple->elements()[0].toTensor();
    clean *= std.std();
    conv_hidden = tuple->elements()[1].toTensorList();
    lstm_hidden = tuple->elements()[2].toTensor();

    clean = clean.contiguous();
    std::copy(clean.data_ptr<float>(), clean.data_ptr<float>() + clean.numel(), out);
  }
  if (chunk != 1) {
    in.erase(in.begin(), in.begin() + (chunk - 1) * valid_length);
  }
  return (chunk-1)*valid_length;
}
