#!/bin/bash
set -e
set -x

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

export CC=/usr/bin/clang-6.0
export CXX=/usr/bin/clang++-6.0
export CFLAGS="-fPIC"
export CXXFLAGS="-fcoroutines-ts -std=c++17 -fexceptions -stdlib=libc++ -fPIC"
export LDFLAGS="-stdlib=libc++ -lc++ -lc++abi -lm -ldl"

cd build
cmake -DZLIB_INCLUDE_DIR=$PWD/../externals/zlib_out/include -DZLIB_LIBRARY=$PWD/../externals/zlib_out/libz.a -DOPENSSL_ROOT_DIR=$PWD/../externals/openssl-out/ -DGTEST_LIBRARY=$PWD/../externals/gtest_out/lib/libgtest.a -DGTEST_INCLUDE_DIR=$PWD/../externals/googletest/googletest/include -DGTEST_MAIN_LIBRARY=$PWD/../externals/gtest_out/lib/libgtest_main.a .. 
make --quiet

cd app/vds_web_server
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib/
./vds_web_server server service --root-folder ~/.vds --web ${DIR}/www/ -lm \* -ll trace
