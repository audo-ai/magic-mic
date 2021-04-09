# Magic Mic

This is the open source component of an app created by the folks at [audo.ai](https://audo.ai/) to provide easy access to a realtime version of our custom machine learning based noise removal tool (our main product is an audio denoising API). Just run the app, switch the microphone in whatever audio consuming app you are using (zoom, discord, google meet) to Magic Mic and you're off! This is still in **early** alpha and so is only available on linux for now. This is still in active development and so bug fixes and new features will be coming, along with OS X and Windows support in the future.

## Open Source
Our custom denoising model is proprietary and so at the moment it is not possible for individuals outside of audo to build this app from source. We plan to add support for building with something like [rnnoise](https://jmvalin.ca/demo/rnnoise/) in the future so anyone can contribute and build the app. 

## Structure
With respect to building, this project has 3 components. First, there is the code in `src-native` which interacts with the audio system and actually creates the virtual microphone and does the denoising. Then there is the [tauri](https://tauri.studio/en/) code in `src-tauri` which deals with creating the system webview and interacting with the frontend code. The naming of these directories is somewhat misleading, because both the code in `src-native` and the code in `src-tauri` are compiled to native code. Additionaly, there is `src-web` which contains a `create-react-app` project which is displayed by tauri. 

## Building
### `src-native`
As mentioned, at the moment this can only be built by us at audo because it relies on our proprietary denoiser. We plan on providing the option to use completely opensource components in the future.

The rough outline for building is:
```sh
mkdir build
cd build
cmake -AUDIO_PROCESSOR_MODULE=... -DVIRTMIC_ENGINE="PIPESOURCE" ..
make bundle_tauri # Copies files and libs over to the src-tauri directory
```
"PIPESOURCE" is the only virtmic engine available at the moment; in the future this may change for other platform or if we implement a custom pulseaudio module

## Frontend
First you need to install [tauri](https://tauri.studio/en/). Then you need to run `yarn` from the root directory and the `src-web` directory. From there, running the app should be as simple as running `yarn tauri dev`, however tauri has some bugs currently so you need to run
```sh
# No rust logging
SERVER_PATH="$PATH_TO_REPO/build/src-native/server-x86_64-unknown-linux-gnu" yarn tauri dev
# With rust logging
RUST_LOG=trace SERVER_PATH="$PATH_TO_REPO/build/src-native/server-x86_64-unknown-linux-gnu" yarn tauri dev
```
