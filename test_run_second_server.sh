#!/bin/bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

${DIR}/cmake-build-debug/app/vds_background/vds_background server init -p 123qwe --root-folder ${DIR}/servers/1 -ll trace -lm \*