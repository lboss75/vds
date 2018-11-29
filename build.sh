#!/bin/bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

rm -rf build
mkdir build
cd build

cmake -DCMAKE_CXX_COMPILER=/usr/bin/clang++-6.0 -DCMAKE_C_COMPILER=/usr/bin/clang-6.0 ..
make

pkill -f vds_web_server

cd app/vds_web_server
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib/
./vds_web_server server service --root-folder ${DIR}/build/app/vds_web_server  --web ${DIR}/www/ -lm \* -ll trace
