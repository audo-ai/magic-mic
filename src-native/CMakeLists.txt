# Configure module
set(BUILD_MODULE OFF CACHE BOOL "should we build module")
set(PULSEAUDIO_SOURCE_TREE "" CACHE PATH "path to the pulse audio source tree")
set(PULSEAUDIO_MODULE_PATH "/usr/lib/pulse-14.0/modules" CACHE PATH "path to the where to install module")
set(VIRTMIC_ENGINE CACHE STRING "which virtual mic engine should be built")
set_property(CACHE VIRTMIC_ENGINE PROPERTY STRINGS "PIPESOURCE")

# Build Server
add_executable(server server/main.cc server/rpc.cc)
set_property(TARGET server PROPERTY CXX_STANDARD 17)
target_include_directories(server PRIVATE common/)

# TODO Not sure why I can't link it PRIVATE
# Well, I can link with private but only if literally everything in this file is
# also linked with private. I tried that and apparently that keeps pipesource
# from being able to find <torch/script.h> (required from denoiser.h). I'm not
# sure why linking settings mess up includes, but I should figure that out.
target_link_libraries(server nlohmann_json::nlohmann_json spdlog::spdlog)

# Configure specifc virtmic engine
if(${VIRTMIC_ENGINE} STREQUAL "PIPESOURCE")
  add_library(pipesource SHARED pipesource/pipesource_virtual_mic.cc)

  target_include_directories(pipesource PRIVATE denoiser/ common/)
  find_package(PulseAudio REQUIRED)
  target_include_directories(pipesource PRIVATE ${PULSEAUDIO_INCLUDE_DIR})
  target_link_libraries(pipesource ${PULSEAUDIO_LIBRARY})

  # Unfortuanately, I can't figure out how to have the torch targets exported
  # from denoiser, so we have to reinclude the package here 
  find_package(Torch REQUIRED)
  target_link_libraries(pipesource denoiser spdlog::spdlog)

  target_link_libraries(server pipesource)
  target_include_directories(server PRIVATE pipesource/ denoiser/)
  set_property(TARGET server PROPERTY COMPILE_DEFINITIONS USE_PIPESOURCE)
else()
  get_property(TEMP CACHE VIRTMIC_ENGINE PROPERTY STRINGS)
  message(SEND_ERROR
    "VIRTMIC_ENGINE cache entry set to invalid value (VIRTMIC_ENGINE=\"${VIRTMIC_ENGINE}\")"
    "\nOPTIONS ARE: ${TEMP}" )
endif()

if (${BUILD_MODULE})
  message(WARNING "Module not yet integrated with anything")
  
  add_library(module-magic-mic MODULE
    module/module-magic-mic.cc
    module/module_class.cc)
  set_target_properties(module-magic-mic PROPERTIES PREFIX "")

  find_package(PulseAudio REQUIRED)
  target_include_directories(module-magic-mic
    PRIVATE /home/gabe/code/audo/pulseaudio/src
    PRIVATE /home/gabe/code/audo/pulseaudio
    PRIVATE ${PULSEAUDIO_INCLUDE_DIR})

  add_compile_definitions(HAVE_CONFIG_H __INCLUDED_FROM_PULSEAUDIO)

  target_compile_options(module-magic-mic PRIVATE -fpermissive)

  target_include_directories(module-magic-mic PRIVATE denoiser/)
  target_link_libraries(module-magic-mic denoiser)

  install(TARGETS module-magic-mic DESTINATION ${PULSEAUDIO_MODULE_PATH})
endif()
