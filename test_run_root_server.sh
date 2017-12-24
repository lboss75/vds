#!/bin/bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

rm -rf ${DIR}/servers

${DIR}/cmake-build-debug/app/vds_background/vds_background server root -p 123qwe -P 8050 --root-folder ${DIR}/servers/0 -ll trace -lm \*
tail ${DIR}/servers/0/.vds/vds.log

${DIR}/cmake-build-debug/app/vds_background/vds_background server start --root-folder ${DIR}/servers/0 -ll trace -lm \*
