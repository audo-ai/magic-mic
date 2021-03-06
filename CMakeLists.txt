cmake_minimum_required(VERSION 3.10)

# set the project name
project(magic-mic)

# Dependencies
include(FetchContent)

# json
# as explained, in its readme, this repo is unecessarily large, but, for now
# we'll leave it as is. There are alternatives (again, as explained in its
# readme), but this will do for now
FetchContent_Declare(json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_SHALLOW "ON"
  GIT_TAG v3.7.3)

FetchContent_GetProperties(json)
if(NOT json_POPULATED)
  FetchContent_Populate(json)

  set(JSON_BuildTests OFF CACHE BOOL "Build json test, we don't want to do that")
  add_subdirectory(${json_SOURCE_DIR} ${json_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

FetchContent_Declare(libtorch
  URL https://download.pytorch.org/libtorch/cpu/libtorch-cxx11-abi-shared-with-deps-1.8.0%2Bcpu.zip
  URL_HASH SHA224=fa02e8a38b8c0466559b22fb8d5f1b21e9e4b435c4cdaefd2d83b36c)

if (NOT libtorch_POPULATED)
  FetchContent_Populate(libtorch)

  list(APPEND CMAKE_PREFIX_PATH ${libtorch_SOURCE_DIR})
endif()

FetchContent_Declare(spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG v1.8.2)

if (NOT spdlog_POPULATED)
  FetchContent_Populate(spdlog)

  option(SPDLOG_BUILD_SHARED "" ON)
  add_subdirectory(${spdlog_SOURCE_DIR} ${spdlog_BINARY_DIR})
endif()

add_subdirectory(src-native)

if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  #TOOD MAke depend on server
  set_target_properties(server PROPERTIES SUFFIX "-x86_64-unknown-linux-gnu")
  set(LIBS_TO_COPY "libpipesource,libspdlog,libdenoiser,libtorch,libtorch_cpu,libc10,libgomp-75eea7e8.so.1")
  add_custom_target(bundle_tauri ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory "runtime_libs"
    COMMAND ../scripts/get_shared_library_deps.sh $<TARGET_FILE:server> "runtime_libs" "${LIBS_TO_COPY}")

  install(TARGETS server DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}/src-tauri/native/")
else()
  message(FATAL_ERROR "Building on anything but linux is not yet supported")
endif() 

install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/runtime_libs/"
  DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}/src-tauri/native/runtime_libs"
  USE_SOURCE_PERMISSIONS)
