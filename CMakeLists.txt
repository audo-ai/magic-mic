cmake_minimum_required(VERSION 3.10)

# set the project name
project(magic-mic)

set(LIBDENOISER_DIR "" CACHE PATH "path to libdenoiser project")

# Dependencies
include(FetchContent)
include(cmake/formatting.cmake)

# json
# as explained, in its readme, this repo is unecessarily large, but, for now
# we'll leave it as is. There are alternatives (again, as explained in its
# readme), but this will do for now
FetchContent_Declare(json
  GIT_REPOSITORY https://github.com/ArthurSonzogni/nlohmann_json_cmake_fetchcontent
  GIT_SHALLOW "ON"
  GIT_TAG v3.7.3)

FetchContent_GetProperties(json)
if(NOT json_POPULATED)
  FetchContent_Populate(json)

  set(JSON_BuildTests OFF CACHE BOOL "Build json test, we don't want to do that")
  add_subdirectory(${json_SOURCE_DIR} ${json_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

FetchContent_Declare(spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_SHALLOW "ON"
  GIT_TAG v1.8.2)

if (NOT spdlog_POPULATED)
  FetchContent_Populate(spdlog)

  option(SPDLOG_BUILD_SHARED "" ON)
  add_subdirectory(${spdlog_SOURCE_DIR} ${spdlog_BINARY_DIR})
endif()

FetchContent_Declare(tray
  GIT_REPOSITORY https://github.com/audo-ai/tray.git
  GIT_TAG 9a3c98520973a39d8c7ad5609bad96f20405c058)

if (NOT tray_POPULATED)
  FetchContent_Populate(tray)

  add_subdirectory(${tray_SOURCE_DIR} ${tray_BINARY_DIR})
endif()

# Needs to be after fetch contents so _deps exists
add_subdirectory(${LIBDENOISER_DIR} "./_deps/libdenoiser-build")

add_subdirectory(src-native)

file(GLOB_RECURSE ALL_CODE_FILES
  ${PROJECT_SOURCE_DIR}/src-native/**/*.[ch]pp
  )
message(WARN ${ALL_CODE_FILES})
clang_format(TARGET_NAME ${ALL_CODE_FILES})

if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  #TOOD MAke depend on server
  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")

  set_target_properties(server PROPERTIES SUFFIX "-x86_64-unknown-linux-gnu")
  set(LIBS_TO_COPY "libpipesource,libspdlog,libgomp")
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
