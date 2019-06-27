#!/bin/bash

set -e
set -x

ARCH=x86_64
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

export NDK=~/Android/Sdk/android-ndk-r19c

rm -rf ${DIR}/build-toolchain
#$NDK/build/tools/make_standalone_toolchain.py \
#  --arch ${ARCH} \
#  --api 21 \
#  --stl=libc++ \
#  --install-dir=${DIR}/build-toolchain


# Add the standalone toolchain to the search path.
#export ANDROID_NDK=${DIR}/build-toolchain
export PATH=$NDK/toolchains/llvm/prebuilt/linux-x86_64/bin:$PATH

#ls ${ANDROID_NDK}/bin/

# Tell configure what tools to use.
export AS=x86_64-linux-android21-clang
export CC=x86_64-linux-android21-clang
export CXX=x86_64-linux-android21-clang++
export LD=x86_64-linux-android21-clang


export CFLAGS="-fPIC -I/usr/include/arm-linux-gnueabihf/"
export CXXFLAGS="-fcoroutines-ts -std=c++1z -fPIC -fno-cxx-exceptions -DLIBCXXABI_HAS_NO_EXCEPTIONS"
export LDFLAGS="-lstdc++ -Wl"
cd ${DIR}/externals

svn -q co http://llvm.org/svn/llvm-project/llvm/trunk llvm

cd llvm/projects
svn -q co http://llvm.org/svn/llvm-project/libcxx/trunk libcxx
svn -q co http://llvm.org/svn/llvm-project/libcxxabi/trunk libcxxabi
cd ..

set +e
rm -rf build
set -e

mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr ..
make --quiet cxx
make --quiet install-cxx install-cxxabi
export CXXFLAGS="-fcoroutines-ts -std=c++1z -stdlib=libc++ -fPIC -fno-cxx-exceptions -DLIBCXXABI_HAS_NO_EXCEPTIONS"
export LDFLAGS="-stdlib=libc++ -lc++ -lc++abi -lm -ldl"
cd ../..

set +e
git clone --depth=1 --single-branch --branch OpenSSL_1_1_1-stable --quiet https://github.com/openssl/openssl.git
rm -rf openssl-out
set -e

export CXXFLAGS="-fcoroutines-ts -std=c++1z -stdlib=libc++ -fPIC -fno-cxx-exceptions -DLIBCXXABI_HAS_NO_EXCEPTIONS"
export LDFLAGS="-stdlib=libc++ -lc++ -lc++abi -lm -ldl"

cd openssl
./Configure linux-armv4 no-tests no-asm --prefix=$PWD/../openssl-out --openssldir=$PWD/../openssl-out
make clean
make --quiet
make --quiet install
cd ..

set +e
git clone --depth=1 --quiet https://github.com/madler/zlib.git
rm -rf zlib_out
set -e

mkdir zlib_out
cd zlib_out
cmake ../zlib
make --quiet
make --quiet install
cd ..

set +e
git clone --depth=1 --quiet https://github.com/google/googletest.git
rm -rf gtest_out
set -e

mkdir gtest_out
cd gtest_out
cmake -Dgtest_force_shared_crt=ON ../googletest/googletest
make --quiet
cd ../..

#build VDS

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
