#!/bin/bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

rm -rf ${DIR}/servers

${DIR}/cmake-build-debug/app/vds_background/vds_background server root -l vadim@iv-soft.ru -p 123 --root-folder /home/vadim/projects/vds.git/servers/0 -ll trace -lm \*
tail ${DIR}/servers/0/.vds/vds.log

${DIR}/cmake-build-debug/app/vds_background/vds_background server start --root-folder ${DIR}/servers/0 -ll trace -lm \*
