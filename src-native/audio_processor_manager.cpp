#include <filesystem>

#include "spdlog/spdlog.h"

#include <dlfcn.h>

#include "audio_processor.hpp"
#include "audio_processor_manager.hpp"

namespace fs = std::filesystem;

AudioProcessorManager::AudioProcessorManager(string dir) {
  auto logger = spdlog::get("server");

  for (const auto &entry : fs::directory_iterator(dir)) {
    void *audio_processor_lib = dlopen(entry.path().c_str(), RTLD_LAZY);
    if (!audio_processor_lib) {
      logger->error("Cannot load Audio Processor from {}: {}",
                    entry.path().c_str(), dlerror());
      continue;
    }
    // reset errors
    dlerror();
    create_audio_processor_t *create_audio_processor =
        (create_audio_processor_t *)dlsym(audio_processor_lib, "create");
    const char *dlsym_error = dlerror();
    if (dlsym_error) {
      logger->error("Cannot load Audio Processor create symbol from {}: {}",
                    entry.path().c_str(), dlsym_error);
      continue;
    }

    destroy_audio_processor_t *destroy_audio_processor =
        (destroy_audio_processor_t *)dlsym(audio_processor_lib, "destroy");
    dlsym_error = dlerror();
    if (dlsym_error) {
      logger->error("Cannot load Audio Processor destroy symbol: {}",
                    dlsym_error);
      continue;
    }
    AudioProcessor *ap = create_audio_processor();
    logger->info("Loaded audioprocessor from {}: {}", entry.path().c_str(),
                 ap->get_info().name);
    aps.push_back(shared_ptr<AudioProcessor>(
        ap, [audio_processor_lib, destroy_audio_processor](AudioProcessor *ap) {
          destroy_audio_processor(ap);
          dlclose(audio_processor_lib);
        }));
  }

  current = 0;
}
