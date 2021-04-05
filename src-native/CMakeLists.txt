# Configure module
set(BUILD_MODULE OFF CACHE BOOL "should we build module")
set(PULSEAUDIO_SOURCE_TREE "" CACHE PATH "path to the pulse audio source tree")
set(PULSEAUDIO_MODULE_PATH "/usr/lib/pulse-14.0/modules" CACHE PATH "path to the where to install module")
set(VIRTMIC_ENGINE CACHE STRING "which virtual mic engine should be built")
set_property(CACHE VIRTMIC_ENGINE PROPERTY STRINGS "PIPESOURCE")

# Build Server
add_executable(server server/main.cpp server/rpc.cpp)
set_property(TARGET server PROPERTY CXX_STANDARD 17)
target_include_directories(server PRIVATE common/ ${tray_INCLUDE_DIR})

# TODO Not sure why I can't link it PRIVATE
# Well, I can link with private but only if literally everything in this file is
# also linked with private. I tried that and apparently that keeps pipesource
# from being able to find <torch/script.h> (required from denoiser.h). I'm not
# sure why linking settings mess up includes, but I should figure that out.
target_link_libraries(server nlohmann_json::nlohmann_json spdlog::spdlog tray)

# Configure specifc virtmic engine
if(${VIRTMIC_ENGINE} STREQUAL "PIPESOURCE")
  add_library(pipesource SHARED pipesource/pipesource_virtual_mic.cpp)

  target_include_directories(pipesource PRIVATE common/)
  find_package(PulseAudio REQUIRED)
  target_include_directories(pipesource PRIVATE ${PULSEAUDIO_INCLUDE_DIR})
  target_link_libraries(pipesource PRIVATE ${PULSEAUDIO_LIBRARY})

  target_link_libraries(pipesource PRIVATE spdlog::spdlog)
  # We don't want to link with denoiser (because of shared vs. static library
  # problems, but we do want dependencies to, and we do need its includes, so we
  # do this. Maybe we could just link the server with denoiser, but this way
  # server doesn't have to worry about these dependencies
  target_link_libraries(pipesource INTERFACE denoiser)
  target_include_directories(pipesource PRIVATE "$<TARGET_PROPERTY:denoiser,INCLUDE_DIRECTORIES>")

  target_link_libraries(server pipesource)
  target_include_directories(server PRIVATE pipesource/ denoiser)
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
    module/module-magic-mic.cpp
    module/module_class.cpp)
  set_target_properties(module-magic-mic PROPERTIES PREFIX "")

  find_package(PulseAudio REQUIRED)
  target_include_directories(module-magic-mic
    PRIVATE /home/gabe/code/audo/pulseaudio/src
    PRIVATE /home/gabe/code/audo/pulseaudio
    PRIVATE ${PULSEAUDIO_INCLUDE_DIR})

  add_compile_definitions(HAVE_CONFIG_H __INCLUDED_FROM_PULSEAUDIO)

  target_compile_options(module-magic-mic PRIVATE -fpermissive)

  target_include_directories(module-magic-mic PRIVATE)

  install(TARGETS module-magic-mic DESTINATION ${PULSEAUDIO_MODULE_PATH})
endif()
