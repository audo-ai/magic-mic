# Native Code

This is the directory containing all of the native code for project-x.

## Dependencies
### spdlog
The staticly linked system spdlog doesn't link with dynamic libraries, so you
need to download the source and build it yourself. You need to set
`SPDLOG_BUILD_SHARED=ON`. You don't need to mess with fmt at all. Then just make
sure `spdlog_DIR` is set to the local build when you build this. Also you might
want to set `CMAKE_INSTALL_PREFIX` to something like `build/prefix`.

## Module
This is a c++ implementation of a pulseaudio module. Puleaudio does not really
support c++ plugins, so that leads to a bit of a hack (just having
to wrap imports in `export "C"` because pulse doesn't do it for us).

### Building
This module requires the denosier. Additionally it requires a pulseaudio source
tree to build with (to give it access to internal headers). Use the cmake cache
variables to configure it appropriatly.

## Denoiser
A library that will hopefully be used as a cross platform denosier engine. Still
in progress. Building setting `-DCMAKE_PREFIX_PATH` to the path to libtorch.

## pipesource-mvp
Start by loading a module-pipe-source with correct sample spec
```
pactl load-module module-pipe-source source_name=virtmic file=$FILE
format=float32le rate=16384 channels=1
```

Then run the app
```
./pipesource-mvp /tmp/virtmic
```
