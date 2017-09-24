#!/bin/bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

kill `cat ${DIR}/servers/0/.vds/vds_background.pid`
kill `cat ${DIR}/servers/1/.vds/vds_background.pid`
kill `cat ${DIR}/servers/2/.vds/vds_background.pid`
kill `cat ${DIR}/servers/3/.vds/vds_background.pid`
kill `cat ${DIR}/servers/4/.vds/vds_background.pid`

rm -rf ${DIR}/servers
rm -rf ${DIR}/clients
rm ${DIR}/README.md

echo "creating server"
${DIR}/build/app/vds_background/vds_background server root -p 123qwe --root-folder ${DIR}/servers/0

echo "starting server"
${DIR}/build/app/vds_background/vds_background server start --root-folder ${DIR}/servers/0 -ll trace -lm "*"

build/app/vds_node/vds_node node install -l root -p 123qwe --root-folder ${DIR}/servers/1
${DIR}/build/app/vds_background/vds_background server start --root-folder ${DIR}/servers/1 -ll trace -lm "*" -P 8051

build/app/vds_node/vds_node node install -l root -p 123qwe --root-folder ${DIR}/servers/2
${DIR}/build/app/vds_background/vds_background server start --root-folder ${DIR}/servers/2 -ll trace -lm "*" -P 8052

build/app/vds_node/vds_node node install -l root -p 123qwe --root-folder ${DIR}/servers/3
${DIR}/build/app/vds_background/vds_background server start --root-folder ${DIR}/servers/3 -ll trace -lm "*" -P 8053

build/app/vds_node/vds_node node install -l root -p 123qwe --root-folder ${DIR}/servers/4
${DIR}/build/app/vds_background/vds_background server start --root-folder ${DIR}/servers/4 -ll trace -lm "*" -P 8054

echo "upload file"
build/app/vds_node/vds_node file upload -l root -p 123qwe -f ${DIR}/README.md --root-folder ${DIR}/clients/0

echo "download file"
build/app/vds_node/vds_node file download -l root -p 123qwe -f ${DIR}/README.md.new -n README.md --root-folder ${DIR}/clients/0

rem https://localhost:8050/vds/dump_state
