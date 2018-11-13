#!/bin/bash

ARCH=x86_64

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

git clone https://github.com/openssl/openssl.git

# Add the standalone toolchain to the search path.
export ANDROID_NDK=${DIR}/../build-toolchain
export PATH=${ANDROID_NDK}/bin:$PATH

# Tell configure what tools to use.
export AS=clang
export CC=clang
export CXX=clang++
export LD=clang

cd openssl
./Configure android-${ARCH} --prefix=${DIR}/openssl-debug --openssldir=${DIR}/openssl-debug
make
make install
