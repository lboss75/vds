#!/bin/bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

${DIR}/cmake-build-debug/app/vds_background/vds_background server start --root-folder ${DIR}/servers/0 -ll trace -lm \*
