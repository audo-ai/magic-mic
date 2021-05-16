cmake_minimum_required(VERSION 3.10)
# For static modules we need to update the server target's linked libs but
# server is not built in this toplevel dir
cmake_policy(SET CMP0079 NEW)

#[[
Howdy, here's the low down on the build system:

Ultimately we are trying to build an as self contained as possible desktop app bundled inside of an appimage (we're only supporting linux anyways right now). Additionally, we want to support a module system that allows audio processors to be shipped with the app and used on the fly. These two constraints are what have dictated the structure of the build system the most.

Building the desktop app:
    Static linking as much as possible would be ideal for this situation, but it is
    often hard to get or build the static versions of libraries and get everything
    to play nicely. So, in those cases we just ship the shared library and
    LD_PRELOAD it. To add a shared library that should be shipped with a target, the
    `BUNDLED_LIBS` property is set on the target with the name of the library (as it
    would appear in eg. ldd). This is then passed to our `get_shared_libraries`
    script which parses ldd output and copies over the library to a local directory
    to be bundled later. I recently learned that cmake seems to keep track of all of
    these libraries so it may be possible to use a `$<TARGET_FILE:>` query or
    something to just get the local file from cmake. That would be nice to not
    rely on the shotty script I wrote.

    TODO: See if we can replace `get_shared_libraries` with `$<TARGET_FILE:>` or something

    The actual app bundling is done by tauri. The tauri config is where all of
    the things you want to bundle with your app are listed, so what we do is
    create a `native` directory tree in the `src-tauri` tree so that the tauri
    bundler can reference all our native stuff. Right now we create a tree like
    ./src-tauri/native/
    ├── runtime_libs
    │   ├── audioprocs
    │   │   └── librnnoise-audioproc.so
    │   └── preload
    │       ├── libnotifymm-1.0.so.7
    │       ├── libnotify.so.4
    │       └── librnnoise.so.0
    └── server-x86_64-unknown-linux-gnu

    where the audioproc libs are in audioprocs and the library deps are in
    preload. Tauri then handles the rest of the bundling into an appimage and
    stuff. Also, tauri expects exes to have the triplet suffix, so we add that
    to our server exe. If we ever add platforms, this is going to need a lot
    more thought.

Distributing Modules:
    The way we distribute an arbitrary number of modules is that every module
    just appends its target to the `AUDIOPROCS` variable using the
    `append_audioproc` function. The target can also have the `BUNDLED_LIBS`
    variable set. We then iterate over this list creating a
    `bundle_tauri-target` target for each target which copies over its bundled
    libs and copies the library to the audioprocs lib. We make all of these
    dependencies of the main `bundle_tauri` target. You can also link your
    module into the main executable if you like. To make things as uniform as
    possible, this behavior is only handled within CMake, there is no speacial
    code to handle staticaly linked modules. A stub shared library which
    references symbols expected to be found in the main exe is generated and used
    just like other module shared libraries which actually contain logic. To
    enable this behavior, do everything just like a normal module, and
    additoinaly make sure the `POSITION_INDEPENDENT_CODE` property is set to off
    and the `SYMBOL_PREFIX` is set such that the module defines the create and
    destroy functions `$SYMBOL_PREFIX_create` and `$SYMBOL_PREFIX_destroy`.

    CMake Note: This all works because it seems like pretty much everything in
    cmake is just a string. Even though it sort of seems like targets are these
    speacial objects, they're really just strings and the speacial stuff is just
    dictionary lookup to find the object somewhere else (these are all black box
    guesses, not confident about any of this). This means we can just create
    lists of strings representing targets and they will resolve to the right
    thing when used in functions or generator expressions or whatever. Also,
    note a useful way to debug target dependency stuff (and also just a nice
    thing to know is available) is running `cmake --graphviz=graph.dot
    ...`. This gives you dep graph, but you'll probably need to modify the
    output so just check
    https://cmake.org/cmake/help/latest/module/CMakeGraphVizOptions.html. You
    don't just modify the variables, you have to create a new file.
# ]]

# set the project name
project(magic-mic)

set(CMAKE_CXX_FLAGS "-Wall")

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

set(AUDIOPROCS)
macro(append_audioproc target)
  list(APPEND AUDIOPROCS ${target})
  set(AUDIOPROCS ${AUDIOPROCS})
  set(AUDIOPROCS
      ${AUDIOPROCS}
      PARENT_SCOPE)
endmacro()

add_subdirectory(src-native)

file(GLOB_RECURSE ALL_CODE_FILES ${PROJECT_SOURCE_DIR}/src-native/*.[ch]pp)
clang_format(format-cpp ${ALL_CODE_FILES})
cmake_format(format-cmake CMakeLists.txt src-native/CMakeLists.txt)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  # TOOD MAke depend on server
  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")

  get_target_property(MAIN_LIBS_TO_COPY server BUNDLED_LIBS)

  set_target_properties(server PROPERTIES SUFFIX "-x86_64-unknown-linux-gnu")
  # Through out here I'm not sure if `add_custom_target` or `add_custom_command`
  # is more appropriate
  add_custom_target(
    bundle_tauri_init ALL
    COMMAND ${CMAKE_COMMAND} -E rm -Rf
            "${CMAKE_CURRENT_SOURCE_DIR}/src-tauri/native/runtime_libs"
    COMMAND ${CMAKE_COMMAND} -E rm -Rf "runtime_libs"
    COMMAND ${CMAKE_COMMAND} -E make_directory "runtime_libs"
    COMMAND ${CMAKE_COMMAND} -E make_directory "runtime_libs/preload"
    COMMAND ${CMAKE_COMMAND} -E make_directory "runtime_libs/audioprocs"
    BYPRODUCTS "${CMAKE_CURRENT_BINARY_DIR}/runtime_libs")

  # Handle modules
  set(AUDIOPROC_BUNDLE_TARGETS)
  foreach(AUDIOPROC ${AUDIOPROCS})
    message(STATUS "Processing Audio Processor \"${AUDIOPROC}\"")
    get_property(LIBS_TO_COPY_DEFD TARGET ${AUDIOPROC} PROPERTY BUNDLED_LIBS DEFINED)
    if (${LIBS_TO_COPY_DEFD})
      get_target_property(LIBS_TO_COPY "${AUDIOPROC}" BUNDLED_LIBS)
    else()
      set(LIBS_TO_COPY "")
    endif()

    get_target_property(AP_PIC "${AUDIOPROC}" POSITION_INDEPENDENT_CODE)
    if (NOT ${AP_PIC})
      # I think add_custom_command is appropriate here because I am generating
      # an output file that will be used as a source file for a subsequent
      # command
      get_target_property(SYMBOL_PREFIX "${AUDIOPROC}" SYMBOL_PREFIX)
      add_custom_command(OUTPUT ${AUDIOPROC}_static_module.cpp
	COMMAND sed --quiet "s/SYMBOL_PREFIX/${SYMBOL_PREFIX}/g; w ${AUDIOPROC}_static_module.cpp" ${CMAKE_CURRENT_SOURCE_DIR}/src-native/static_module_stub.cpp.in 
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src-native/static_module_stub.cpp.in
	VERBATIM)

      add_library(${AUDIOPROC}_module MODULE ${AUDIOPROC}_static_module.cpp)
      target_include_directories(${AUDIOPROC}_module PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src-native)

      target_link_libraries(server -export-dynamic ${AUDIOPROC} -u${SYMBOL_PREFIX}_create)

      # So that the generated shared library is bundled and not the static one
      set(AUDIOPROC ${AUDIOPROC}_module)
    endif()

    add_custom_target(
      bundle_tauri_${AUDIOPROC}
      COMMAND
      ../scripts/get_shared_library_deps.sh "$<TARGET_FILE:${AUDIOPROC}>"
      "runtime_libs/preload" "${LIBS_TO_COPY}"
      COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:${AUDIOPROC}>"
      "${CMAKE_CURRENT_BINARY_DIR}/runtime_libs/audioprocs"
      DEPENDS bundle_tauri_init ${AUDIOPROC})
    list(APPEND AUDIOPROC_BUNDLE_TARGETS "bundle_tauri_${AUDIOPROC}")

  endforeach()

  add_custom_target(
    bundle_tauri_server
    COMMAND ../scripts/get_shared_library_deps.sh $<TARGET_FILE:server>
            "runtime_libs/preload" "${MAIN_LIBS_TO_COPY}"
    DEPENDS server bundle_tauri_init)

  add_custom_target(bundle_tauri ALL DEPENDS bundle_tauri_server
                                             ${AUDIOPROC_BUNDLE_TARGETS})

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
