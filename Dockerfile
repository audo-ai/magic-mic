FROM ubuntu:20.10
SHELL ["/bin/bash", "-c"]
# General softare build deps
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y \
    cmake g++ git bash npm curl
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
RUN npm install --global yarn

# magic-mic deps
RUN DEBIAN_FRONTEND=noninteractive apt-get install -y \
    libpulse-dev 

RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y && \
    source $HOME/.cargo/env && \
    rustup update stable && \
    cargo install tauri-bundler --force

# TODO: We probably should pull from github, but this will do for now. Before
# being able to pull from github we need to either include credentials in here
# or make the repo public. I recommend making a local clone of your development
# repo so that docker doesn't include all of the build stuff and just includes
# the code in the working directory.
COPY . /src

RUN cd /src && \
    mkdir build && \
    cd build && \
    cmake -DVIRTMIC_ENGINE="PIPESOURCE" .. && \
    make install -j
RUN cd /src && \
    source $HOME/.cargo/env && \
    npx --no-install yarn
RUN cd /src/src-web && \
    npx --no-install yarn

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
    npx --no-install yarn tauri build