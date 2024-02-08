FROM ubuntu:18.04

RUN apt update && apt install -y \
  git \
  wget \
  gcc \
  g++ \
  ninja-build \
  pkg-config \
  libjack-jackd2-dev \
  libasound2-dev \
  ladspa-sdk \
  libcurl4-openssl-dev \
  libfreetype6-dev \
  libx11-dev \
  libxcomposite-dev \
  libxcursor-dev \
  libxext-dev \
  libxinerama-dev \
  libxrandr-dev \
  libxrender-dev \
  libglu1-mesa-dev \
  mesa-common-dev

# Add cmake
RUN mkdir -p ~/bin && \
  cd ~ && \
  wget https://github.com/Kitware/CMake/releases/download/v3.28.1/cmake-3.28.1-linux-x86_64.tar.gz && \
  tar xf cmake-3.28.1-linux-x86_64.tar.gz && cd cmake-3.28.1-linux-x86_64/bin && \
  ln -s $PWD/cmake ~/bin/cmake && \
  echo PATH=~/bin:$PATH >> ~/.bashrc
