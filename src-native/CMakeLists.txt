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
set(AUDIOPROC_CMAKES
    CACHE STRING "semicolon seperated list of paths to audioproc cmakes")

# Build Server
pkg_check_modules(libnotifymm REQUIRED IMPORTED_TARGET libnotifymm-1.0)
add_executable(server main.cpp rpc.cpp audio_processor_manager.cpp)
target_include_directories(server PRIVATE common/ ${tray_INCLUDE_DIR})
target_link_libraries(server PkgConfig::libnotifymm)

# TODO Not sure why I can't link it PRIVATE Well, I can link with private but
# only if literally everything in this file is also linked with private. I tried
# that and apparently that keeps pipesource from being able to find
# <torch/script.h> (required from denoiser.h). I'm not sure why linking settings
# mess up includes, but I should figure that out.
target_link_libraries(server dl nlohmann_json::nlohmann_json spdlog::spdlog
                      tray)
set_target_properties(server PROPERTIES BUNDLED_LIBS "libnotifymm,libnotify")

set(MAGIC_MIC ${CMAKE_SOURCE_DIR})
foreach(AP_CMAKE ${AUDIOPROC_CMAKES})
  get_filename_component(AP_NAME ${AP_CMAKE} NAME_WE)
  get_filename_component(AP_FILENAME ${AP_CMAKE} NAME)
  get_filename_component(AP_DIR ${AP_CMAKE} DIRECTORY)
  message(
    "Searching for ${AP_NAME} with config file ${AP_FILENAME} in dir ${AP_DIR}")
  find_package(
    ${AP_NAME}
    REQUIRED
    CONFIG
    CONFIGS
    ${AP_FILENAME}
    PATHS
    "${AP_DIR}"
    NO_DEFAULT_PATH)
endforeach()

# Configure specifc virtmic engine
if(${VIRTMIC_ENGINE} STREQUAL "PIPESOURCE")
  add_library(pipesource STATIC pipesource_virtual_mic.cpp)

  find_package(PulseAudio REQUIRED)
  target_include_directories(pipesource PRIVATE ${PULSEAUDIO_INCLUDE_DIR})
  target_link_libraries(pipesource PRIVATE ${PULSEAUDIO_LIBRARY})

  find_package(Boost REQUIRED)
  target_link_libraries(pipesource PRIVATE Boost::boost)

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
