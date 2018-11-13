#!/bin/bash

ARCH=x86_64

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Add the standalone toolchain to the search path.
export ANDROID_NDK=${DIR}/../build-toolchain
export PATH=${ANDROID_NDK}/../bin:$PATH

# Tell configure what tools to use.
export AS=clang
export CC=clang
export CXX=clang++
export LD=clang
export CFLAGS="-fPIC"

rm -rf ${DIR}/zlib_debug
mkdir ${DIR}/zlib_debug
cd ${DIR}/zlib_debug

cmake ${DIR}/zlib
make
#cmake --build . --target install

