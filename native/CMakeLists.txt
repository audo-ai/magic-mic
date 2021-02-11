cmake_minimum_required(VERSION 3.10)

# set the project name
project(magic-mic)

# TODO: Maybe eventuall seperate out the CMakeLists.txt into each directory
# Configure denoiser
add_library(denoiser SHARED denoiser/denoiser.cc)

find_package(Torch REQUIRED)
message("${TORCH_LIBRARIES}")
target_link_libraries(denoiser "${TORCH_LIBRARIES}")
set_property(TARGET denoiser PROPERTY CXX_STANDARD 14)

# Configure module
set(BUILD_MODULE ON CACHE BOOL "should we build module")
set(PULSEAUDIO_SOURCE_TREE "" CACHE PATH "path to the pulse audio source tree")
set(PULSEAUDIO_MODULE_PATH "/usr/lib/pulse-14.0/modules" CACHE PATH "path to the where to install module")

if (${BUILD_MODULE})
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

set(BUILD_PIPESOURCE_MVP CACHE BOOL "should we build pipe source mvp")
if (${BUILD_PIPESOURCE_MVP})
  add_executable(pipesource-mvp pipesource-mvp/main.cc pipesource-mvp/app.cc)

  target_include_directories(pipesource-mvp PRIVATE denoiser/)
  find_package(PulseAudio REQUIRED)
  target_include_directories(pipesource-mvp PRIVATE ${PULSEAUDIO_INCLUDE_DIR})
  target_link_libraries(pipesource-mvp ${PULSEAUDIO_LIBRARY})

  target_link_libraries(pipesource-mvp denoiser)
endif()
