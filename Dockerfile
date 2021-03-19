# Must be run with DOCKER_BUILDKIT=1 and --ssh default
# syntax=docker/dockerfile:experimental
FROM ubuntu:18.04

SHELL ["/bin/bash", "-c"]
# For some reason, things fail to install without this command
RUN apt-get update && apt-get install -y --no-install-recommends apt-utils

# General softare build deps
RUN DEBIAN_FRONTEND=noninteractive apt-get install -y \
    git bash curl snapd wget software-properties-common

# Get newer version of g++
RUN add-apt-repository ppa:ubuntu-toolchain-r/test && \
    apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y gcc-10 g++-10

# Need a newer cmake than ubuntu provides (for FetchContent)
RUN wget https://github.com/Kitware/CMake/releases/download/v3.19.5/cmake-3.19.5-Linux-x86_64.sh && sh cmake-3.19.5-Linux-x86_64.sh -- --skip-license

# Tauri deps
RUN DEBIAN_FRONTEND=noninteractive apt-get install -y \
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
RUN \. ~/.nvm/nvm.sh && nvm install 10.19

# Install cargo and rust through rustup
RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y && \
    source $HOME/.cargo/env && \
    rustup update stable && \
    cargo install tauri-bundler --force

# magic-mic deps
RUN DEBIAN_FRONTEND=noninteractive apt-get install -y \
    libpulse-dev libeigen3-dev libopenmpi-dev libgomp1

ARG PYTORCH_PREFIX
COPY $PYTORCH_PREFIX /pytorch

RUN mkdir -p -m 0600 ~/.ssh && ssh-keyscan github.com >> ~/.ssh/known_hosts
RUN --mount=type=ssh git clone git@github.com:audo-ai/libdenoiser.git /libdenoiser
RUN --mount=type=ssh git clone git@github.com:audo-ai/magic-mic /src

RUN cd /src && \
    mkdir build && \
    cd build && \
    cmake -DCMAKE_CXX_COMPILER=`which g++-10` \
    	  -DCMAKE_PREFIX_PATH=/pytorch \
    	  -DLIBDENOISER_DIR=/libdenoiser \
	  -DVIRTMIC_ENGINE="PIPESOURCE" .. && \
    make install -j 4

RUN \. ~/.nvm/nvm.sh && nvm use 10.19 && npm install --global yarn

RUN cd /src && \
    source $HOME/.cargo/env && \
    \. ~/.nvm/nvm.sh && nvm use 10.19 && yarn

RUN cd /src/src-web && \
    \. ~/.nvm/nvm.sh && nvm use 10.19 && yarn

# appimagetool seems to want to use fuse for some reason to create the appimage,
# but it doesn't need it. build_appimage.sh checks if fuse is usable with lsmod
# | grep fuse. fuse is annoying to use on docker so we'll just shim lsmod not to
# list fuse. I think the reason it lists fuse even though fuse is not usable on
# docker is probably because of the docker sandboxing
RUN mkdir $HOME/lsmod_shim && \
    echo -e '!/bin/bash\necho NOTHING' > $HOME/lsmod_shim/lsmod && \
    chmod +x $HOME/lsmod_shim/lsmod
RUN cd /src && \
    PATH=$HOME/lsmod_shim:$PATH && \
    source $HOME/.cargo/env && \
    \. ~/.nvm/nvm.sh && nvm use 10.19 && yarn tauri build