#!/bin/bash

set -e
set -x

ARCH=x86_64
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

export NDK=~/Android/Sdk/android-ndk-r20

rm -rf ${DIR}/build-toolchain
#$NDK/build/tools/make_standalone_toolchain.py \
#  --arch ${ARCH} \
#  --api 21 \
#  --stl=libc++ \
#  --install-dir=${DIR}/build-toolchain


# Add the standalone toolchain to the search path.
#export ANDROID_NDK=${DIR}/build-toolchain
export PATH=$NDK/toolchains/llvm/prebuilt/linux-x86_64/bin:$PATH

ls $NDK/toolchains/llvm/prebuilt/linux-x86_64/bin

find / | grep 'coroutine'

# Tell configure what tools to use.
export AS=clang
export CC=clang
export CXX=clang++
export LD=clang


#export CFLAGS="-fPIC -I/usr/include/arm-linux-gnueabihf/"
#export CXXFLAGS="-fcoroutines-ts -std=c++1z -fPIC -fno-cxx-exceptions"
#export LDFLAGS="-lstdc++ -Wl -latomic" 
cd ${DIR}/externals

#svn -q co http://llvm.org/svn/llvm-project/llvm/trunk llvm

#cd llvm/projects
#svn -q co http://llvm.org/svn/llvm-project/libcxx/trunk libcxx
#svn -q co http://llvm.org/svn/llvm-project/libcxxabi/trunk libcxxabi
#cd ..

#set +e
#rm -rf build
#set -e

#mkdir build
#cd build
#cmake -DCMAKE_INSTALL_PREFIX=/usr -DLIBCXXABI_ENABLE_EXCEPTIONS=OFF ..
#make --quiet cxx
#make --quiet install-cxx 
#install-cxxabi
#export CXXFLAGS="-fcoroutines-ts -std=c++1z -stdlib=libc++ -fPIC -fno-cxx-exceptions"
#export LDFLAGS="-stdlib=libc++ -lc++ -lm -ldl"
#cd ../..

set +e
git clone --depth=1 --single-branch --branch OpenSSL_1_1_1-stable --quiet https://github.com/openssl/openssl.git
rm -rf openssl-out
set -e

export CFLAGS="-fPIC"
export CXXFLAGS="-fcoroutines-ts -std=c++1z -stdlib=libc++ -fPIC -fno-cxx-exceptions -I$NDK/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/include/c++/v1/"
export LDFLAGS="-stdlib=libc++ -lc++ -lm -ldl"

cd openssl
./Configure linux-armv4 no-tests no-asm --prefix=$PWD/../openssl-out --openssldir=$PWD/../openssl-out
make clean
make 
make install
cd ..

set +e
git clone --depth=1 --quiet https://github.com/madler/zlib.git
rm -rf zlib_out
set -e

mkdir zlib_out
cd zlib_out
cmake ../zlib
make 
make install
cd ..

set +e
git clone --depth=1 --quiet https://github.com/google/googletest.git
rm -rf gtest_out
set -e

mkdir gtest_out
cd gtest_out
cmake -Dgtest_force_shared_crt=ON -DOPENSSL_ROOT_DIR=${DIR}/externals/openssl-out ../googletest/googletest
make 
cd ../..

#build VDS

rm -rf build
mkdir build
cd build

cmake .. \
   -DOPENSSL_ROOT_DIR=${DIR}/externals/openssl-out \
   -DZLIB_INCLUDE_DIR=${DIR}/externals/zlib_debug/include \
   -DZLIB_LIBRARY=${DIR}/externals/zlib_debug/libz.a \
   -DGTEST_LIBRARY=${DIR}/externals/gtest_out/lib/libgtest.a
   -DGTEST_INCLUDE_DIR=${DIR}/externals/googletest/googletest/include
   -DGTEST_MAIN_LIBRARY=${DIR}/externals/gtest_out/lib/libgtest_main.a

make
