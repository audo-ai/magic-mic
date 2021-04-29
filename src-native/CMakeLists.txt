# Configure module
set(BUILD_MODULE
    OFF
    CACHE BOOL "should we build module")
set(PULSEAUDIO_SOURCE_TREE
    ""
    CACHE PATH "path to the pulse audio source tree")
set(PULSEAUDIO_MODULE_PATH
    "/usr/lib/pulse-14.0/modules"
    CACHE PATH "path to the where to install module")
set(VIRTMIC_ENGINE CACHE STRING "which virtual mic engine should be built")
set_property(CACHE VIRTMIC_ENGINE PROPERTY STRINGS "PIPESOURCE")
set(AUDIO_PROCESSOR_USE_RNNOISE
    ON
    CACHE BOOL "use rnnoise as the audio processor")
set(AUDIO_PROCESSOR_MODULE
    CACHE PATH "path to adudio processor shared lib if not using rnnoise")

set(CMAKE_CXX_STANDARD 17)

# Build Server
pkg_check_modules(libnotifymm REQUIRED IMPORTED_TARGET libnotifymm-1.0)
add_executable(server main.cpp rpc.cpp)
target_include_directories(server PRIVATE common/ ${tray_INCLUDE_DIR})
target_link_libraries(server PkgConfig::libnotifymm)

# TODO Not sure why I can't link it PRIVATE Well, I can link with private but
# only if literally everything in this file is also linked with private. I tried
# that and apparently that keeps pipesource from being able to find
# <torch/script.h> (required from denoiser.h). I'm not sure why linking settings
# mess up includes, but I should figure that out.
target_link_libraries(server dl nlohmann_json::nlohmann_json spdlog::spdlog
                      tray)

# Build audio processor
if(${AUDIO_PROCESSOR_USE_RNNOISE})
  message(STATUS "Using rnnoise as audio processor module")
  pkg_check_modules(rnnoise REQUIRED IMPORTED_TARGET rnnoise)
  add_library(audioproc MODULE rnnoise_proc.cpp)
  target_link_libraries(audioproc PkgConfig::rnnoise)
  # This is just so librnnoise can be found by get_shared_library_deps and
  # distributed
  target_link_libraries(server PkgConfig::rnnoise)
elseif(NOT EXISTS ${AUDIO_PROCESSOR_MODULE})
  message(FATAL_ERROR "${AUDIO_PROCESSOR_MODULE} not found")
else()
  execute_process(
    COMMAND ldd ${AUDIO_PROCESSOR_MODULE}
    RESULT_VARIABLE LDD_STATUS
    OUTPUT_QUIET ERROR_QUIET)
  if(${LDD_STATUS} EQUAL 0)
    message(STATUS "Using ${AUDIO_PROCESSOR_MODULE} as audio processor module")
    add_library(audioproc MODULE IMPORTED GLOBAL)
    set_target_properties(audioproc PROPERTIES IMPORTED_LOCATION
                                               "${AUDIO_PROCESSOR_MODULE}")
  else()
    message(FATAL_ERROR "${AUDIO_PROCESSOR_MODULE} is not a shared library")
  endif()
endif()

# Configure specifc virtmic engine
if(${VIRTMIC_ENGINE} STREQUAL "PIPESOURCE")
  add_library(pipesource STATIC pipesource_virtual_mic.cpp)

  find_package(PulseAudio REQUIRED)
  target_include_directories(pipesource PRIVATE ${PULSEAUDIO_INCLUDE_DIR})
  target_link_libraries(pipesource PRIVATE ${PULSEAUDIO_LIBRARY})

  target_link_libraries(pipesource PRIVATE spdlog::spdlog)

  target_link_libraries(server pipesource)
  set_property(TARGET server PROPERTY COMPILE_DEFINITIONS USE_PIPESOURCE)
else()
  get_property(
    TEMP
    CACHE VIRTMIC_ENGINE
    PROPERTY STRINGS)
  message(
    SEND_ERROR
      "VIRTMIC_ENGINE cache entry set to invalid value (VIRTMIC_ENGINE=\"${VIRTMIC_ENGINE}\")"
      "\nOPTIONS ARE: ${TEMP}")
endif()

if(${BUILD_MODULE})
  message(WARNING "Module not yet integrated with anything")

  add_library(module-magic-mic MODULE module/module-magic-mic.cpp
                                      module/module_class.cpp)
  set_target_properties(module-magic-mic PROPERTIES PREFIX "")

  find_package(PulseAudio REQUIRED)
  target_include_directories(
    module-magic-mic
    PRIVATE /home/gabe/code/audo/pulseaudio/src
    PRIVATE /home/gabe/code/audo/pulseaudio
    PRIVATE ${PULSEAUDIO_INCLUDE_DIR})

  add_compile_definitions(HAVE_CONFIG_H __INCLUDED_FROM_PULSEAUDIO)

  target_compile_options(module-magic-mic PRIVATE -fpermissive)

  target_include_directories(module-magic-mic PRIVATE)

  install(TARGETS module-magic-mic DESTINATION ${PULSEAUDIO_MODULE_PATH})
endif()
