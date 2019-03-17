#!/bin/bash

ARCH=x86_64
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

export NDK=~/Android/Sdk/ndk-bundle

rm -rf ${DIR}/build-toolchain
$NDK/build/tools/make_standalone_toolchain.py \
  --arch ${ARCH} \
  --api 21 \
  --stl=libc++ \
  --install-dir=${DIR}/build-toolchain


# Add the standalone toolchain to the search path.
export ANDROID_NDK=${DIR}/build-toolchain
export PATH=${DIR}/build-toolchain/bin:$PATH

# Tell configure what tools to use.
export AS=clang
export CC=clang
export CXX=clang++
export LD=clang
#export CXXFLAGS="-I ${DIR}/build-toolchain/include/c++/4.9.x"

rm -rf build
mkdir build
cd build

cmake .. \
   -DOPENSSL_ROOT_DIR=${DIR}/externals/openssl-debug \
   -DZLIB_INCLUDE_DIR=${DIR}/externals/zlib_debug/include \
   -DZLIB_LIBRARY=${DIR}/externals/zlib_debug/libz.a \
   -DCMAKE_SYSTEM_NAME=Android \
   -DCMAKE_SYSTEM_VERSION=21 \
   -DCMAKE_ANDROID_ARCH_ABI=${ARCH} \
   -DCMAKE_ANDROID_STANDALONE_TOOLCHAIN=${DIR}/build-toolchain

make



