cmake_minimum_required(VERSION 3.10)

# set the project name
project(magic-mic)

# Dependencies
include(FindPkgConfig) # For libnotifymm
include(FetchContent)
include(cmake/formatting.cmake)

FetchContent_Declare(
  json
  GIT_REPOSITORY
    https://github.com/ArthurSonzogni/nlohmann_json_cmake_fetchcontent
  GIT_SHALLOW "ON"
  GIT_TAG v3.7.3)

FetchContent_GetProperties(json)
if(NOT json_POPULATED)
  FetchContent_Populate(json)

  set(JSON_BuildTests
      OFF
      CACHE BOOL "Build json test, we don't want to do that")
  add_subdirectory(${json_SOURCE_DIR} ${json_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_SHALLOW "ON"
  GIT_TAG v1.8.2)

if(NOT spdlog_POPULATED)
  FetchContent_Populate(spdlog)

  add_subdirectory(${spdlog_SOURCE_DIR} ${spdlog_BINARY_DIR})
endif()

FetchContent_Declare(
  tray
  GIT_REPOSITORY https://github.com/audo-ai/tray.git
  GIT_TAG 8b9e733b82a674063fb484a0ed825e01aca34172)

if(NOT tray_POPULATED)
  FetchContent_Populate(tray)

  add_subdirectory(${tray_SOURCE_DIR} ${tray_BINARY_DIR})
endif()

add_subdirectory(src-native)

file(GLOB_RECURSE ALL_CODE_FILES ${PROJECT_SOURCE_DIR}/src-native/*.[ch]pp)
clang_format(format-cpp ${ALL_CODE_FILES})
cmake_format(format-cmake CMakeLists.txt src-native/CMakeLists.txt)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  # TOOD MAke depend on server
  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")

  set_target_properties(server PROPERTIES SUFFIX "-x86_64-unknown-linux-gnu")
  add_custom_target(
    bundle_tauri ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory "runtime_libs"
    COMMAND ${CMAKE_COMMAND} -E rm -Rf
            "${CMAKE_CURRENT_SOURCE_DIR}/src-tauri/native/runtime_libs"
    COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:audioproc>"
            "${CMAKE_CURRENT_BINARY_DIR}/runtime_libs/audioproc.so"
    DEPENDS audioproc server
    BYPRODUCTS "${CMAKE_CURRENT_BINARY_DIR}/runtime_libs")
  add_custom_target(
    install_tauri
    DEPENDS bundle_tauri
    COMMAND
      ${CMAKE_COMMAND} -E copy_directory
      "${CMAKE_CURRENT_BINARY_DIR}/runtime_libs/"
      "${CMAKE_CURRENT_SOURCE_DIR}/src-tauri/native/runtime_libs"
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:server>
            "${CMAKE_CURRENT_SOURCE_DIR}/src-tauri/native/")

else()
  message(FATAL_ERROR "Building on anything but linux is not yet supported")
endif()
