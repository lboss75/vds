#!/bin/bash
set -e
set -x

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

export CC=clang-6.0
export CXX=clang++-6.0
export LD=clang++-6.0

export CFLAGS="-fPIC -fexceptions -I/usr/include/arm-linux-gnueabihf/"
export CXXFLAGS="-fcoroutines-ts -std=c++17 -fexceptions -fPIC"
export LDFLAGS="-lstdc++ -Wl"
cd externals

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
export CXXFLAGS="-fcoroutines-ts -std=c++17 -fexceptions -stdlib=libc++ -fPIC"
export LDFLAGS="-stdlib=libc++ -lc++ -lc++abi -lm -ldl"
cd ../..

set +e
git clone --depth=1 --single-branch --branch OpenSSL_1_1_1-stable --quiet https://github.com/openssl/openssl.git
rm -rf openssl-out
set -e

export CXXFLAGS="-fcoroutines-ts -std=c++17 -fexceptions -stdlib=libc++ -fPIC"
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
