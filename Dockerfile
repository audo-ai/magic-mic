# Must be run with DOCKER_BUILDKIT=1
# syntax=docker/dockerfile:experimental
ARG TARGET=rnnoise
ARG BASE_IMAGE=ubuntu:18.04
FROM $BASE_IMAGE as common

SHELL ["/bin/bash", "-c"]
# For some reason, things fail to install later without this command
RUN apt-get update && apt-get install -y --no-install-recommends apt-utils

# General softare build deps
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y \
    git bash curl snapd wget software-properties-common

# Get newer version of g++ (this may no longer be necessary)
RUN add-apt-repository ppa:ubuntu-toolchain-r/test && \
    apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y gcc-10 g++-10

# Need a newer cmake than ubuntu provides (for FetchContent)
RUN wget https://github.com/Kitware/CMake/releases/download/v3.19.5/cmake-3.19.5-Linux-x86_64.sh && \
    sh cmake-3.19.5-Linux-x86_64.sh -- --skip-license && \
    rm cmake-3.19.5-Linux-x86_64.sh

# Tauri deps
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y \
    build-essential \
    curl \
    wget \
    libssl-dev \
    appmenu-gtk3-module \
    libgtk-3-dev \
    squashfs-tools \
    libdbus-1-dev \
    libwebkit2gtk-4.0-dev

# We need a newer version of node than ubuntu has
RUN wget -qO- https://raw.githubusercontent.com/nvm-sh/nvm/v0.37.2/install.sh | bash
RUN \. ~/.nvm/nvm.sh && nvm install 12.13
RUN \. ~/.nvm/nvm.sh && nvm use 12.13 && npm install --global yarn

# Install cargo and rust through rustup
RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y && \
    source $HOME/.cargo/env && \
    rustup update stable && \
    cargo install tauri-bundler --force

# magic-mic deps
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y \
    libpulse-dev libappindicator3-dev libnotify-dev glib2.0 libgtkmm-3.0 libboost-dev
RUN wget http://ftp.gnome.org/pub/gnome/sources/libnotifymm/0.7/libnotifymm-0.7.0.tar.xz && \
    tar xf libnotifymm-0.7.0.tar.xz && \
    cd libnotifymm-0.7.0 && \
    ./configure && \
    make -j 4 install
RUN git clone --depth=1 https://github.com/xiph/rnnoise.git && \
    cd /rnnoise && \
    ./autogen.sh && \
    ./configure && \
    make install -j 4

# Rust caching ideas from https://blog.mgattozzi.dev/caching-rust-docker-builds/
RUN mkdir -p /src/src-tauri/src
RUN echo "fn main() {}" > /src/src-tauri/dummy.rs
# We might also need a dummy for build.rs but to be completely honest I'm not
# sure what build.rs does
COPY ./src-tauri/src/build.rs /src/src-tauri/src/build.rs
COPY ./src-tauri/Cargo.lock /src/src-tauri
COPY ./src-tauri/Cargo.toml /src/src-tauri
RUN sed -i 's/src\/main.rs/dummy.rs/' /src/src-tauri/Cargo.toml
RUN cd /src/src-tauri && \
    source $HOME/.cargo/env && \
    cargo build --release
RUN sed -i 's/dummy.rs/src\/main.rs/' /src/src-tauri/Cargo.toml

# install src-web deps
COPY ./src-web /src/src-web
RUN cd /src/src-web && \
    \. ~/.nvm/nvm.sh && nvm use 12.13 && yarn

# install tauri cli deps
COPY ./package.json /src
RUN cd /src && \
    source $HOME/.cargo/env && \
    \. ~/.nvm/nvm.sh && nvm use 12.13 && yarn

COPY . /src

# appimagetool seems to want to use fuse for some reason to create the appimage,
# but it doesn't need it. build_appimage.sh checks if fuse is usable with lsmod
# | grep fuse. fuse is annoying to use on docker so we'll just shim lsmod not to
# list fuse. I think the reason it lists fuse even though fuse is not usable on
# docker is probably because of the docker sandboxing
RUN echo -e '!/bin/bash\necho NOTHING' >/usr/local/bin/lsmod && \
    chmod +x /usr/local/bin

FROM common as rnnoise
RUN cd /src && \
    PATH=$HOME/lsmod_shim:$PATH && \
    source $HOME/.cargo/env && \
    \. ~/.nvm/nvm.sh && nvm use 12.13 && \
    mkdir build && \
    cd build && \
    cmake -DCMAKE_CXX_COMPILER=`which g++-10` \
	  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
	  -DAUDIOPROC_CMAKES="$PWD/../src-native/RNNoiseAP.cmake" \
	  -DVIRTMIC_ENGINE="PIPESOURCE" .. && \
    make build_tauri -j $(nproc)

FROM common as audo
RUN cd /src && \
    PATH=$HOME/lsmod_shim:$PATH && \
    source $HOME/.cargo/env && \
    \. ~/.nvm/nvm.sh && nvm use 12.13 && \
    mkdir build && \
    cd build && \
    cmake -C /src-libdenoiser/cache.cmake \
	  -DCMAKE_CXX_COMPILER=`which g++-10` \
	  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
	  -DAUDIOPROC_CMAKES="$PWD/../src-native/RNNoiseAP.cmake;/src-libdenoiser/AudoAP.cmake" \
	  -DVIRTMIC_ENGINE="PIPESOURCE" \
	  -DCMAKE_PREFIX_PATH=/src-libdenoiser/cmake_prefix .. && \
    make build_tauri -j $(nproc)

# Give target a constant name so that the copy works
FROM $TARGET as build
FROM scratch as bin
COPY --from=build /src/src-tauri/target/release/bundle/appimage/magic-mic_0.1.0_amd64.AppImage .
