#!/bin/bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

rm -rf ${DIR}/build
mkdir build
cd build

export CC=/usr/bin/clang-6.0
export CXX=/usr/bin/clang++-6.0

cmake ..
make

cd app/vds_web_server
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib/
./vds_web_server server service --root-folder ${DIR}/build/app/vds_web_server  --web ${DIR}/www/ -lm \* -ll trace
