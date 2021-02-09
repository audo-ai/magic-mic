# Native Code

This is the directory containing all of the native code for project-x.

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
