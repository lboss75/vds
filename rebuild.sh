#!/bin/bash
set -e
set -x

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

export CC=/usr/bin/clang-6.0
export CXX=/usr/bin/clang++-6.0
export CFLAGS="-fPIC"
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
cmake ..
make --quiet cxx
make --quiet install-cxx install-cxxabi
export CXXFLAGS="-fcoroutines-ts -std=c++17 -fexceptions -stdlib=libc++ -fPIC"
export LDFLAGS="-stdlib=libc++ -lc++ -lc++abi -lm -ldl"
cd ../..

set +e
git clone --quiet https://github.com/openssl/openssl.git
rm -rf openssl-out
set -e

cd openssl
./Configure linux-x86_64-clang no-tests --prefix=$PWD/../openssl-out --openssldir=$PWD/../openssl-out
make --quiet
make --quiet install
cd ..

set +e
git clone --quiet https://github.com/madler/zlib.git
rm -rf zlib_out
set -e

mkdir zlib_out
cd zlib_out
cmake ../zlib
make --quiet
make --quiet install
cd ..

set +e
git clone --quiet https://github.com/google/googletest.git
rm -rf gtest_out
set -e

mkdir gtest_out
cd gtest_out
cmake -Dgtest_force_shared_crt=ON ../googletest/googletest
make --quiet
cd ../..
rm -rf build
mkdir build
cd build
cmake -DZLIB_INCLUDE_DIR=$PWD/../externals/zlib_out/include -DZLIB_LIBRARY=$PWD/../externals/zlib_out/libz.a -DOPENSSL_ROOT_DIR=$PWD/../externals/openssl-out/ -DGTEST_LIBRARY=$PWD/../externals/gtest_out/lib/libgtest.a -DGTEST_INCLUDE_DIR=$PWD/../externals/googletest/googletest/include -DGTEST_MAIN_LIBRARY=$PWD/../externals/gtest_out/lib/libgtest_main.a .. 
make --quiet

cd app/vds_web_server
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib/
./vds_web_server server service --root-folder ${DIR}/build/app/vds_web_server  --web ${DIR}/www/ -lm \* -ll trace
