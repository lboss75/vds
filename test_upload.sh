#!/bin/bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

rm -rf ${DIR}/build/servers
rm -rf ${DIR}/build/clients

echo "creating server"
echo "build/app/vds_background/vds_background server root -p 123qwe -r ${DIR}/build/servers/1"
build/app/vds_background/vds_background server root -p 123qwe -r ${DIR}/build/servers/1

echo "starting server"
echo "build/app/vds_background/vds_background server start -r ${DIR}/build/servers/1"
daemon build/app/vds_background/vds_background server start -r ${DIR}/build/servers/1 -ll trace -lm "*"


echo "creating second server"
echo "build/app/vds_node/vds_node node install -l root -p 123qwe -r ${DIR}/build/servers/2"
build/app/vds_node/vds_node node install -l root -p 123qwe -r ${DIR}/build/servers/2 -ll trace -lm "*"

echo "uploading file"
echo "build/app/vds_node/vds_node file upload -l root -p 123qwe -f ${DIR}/build/app/vds_node/vds_node -r ${DIR}/build/clients/2"
build/app/vds_node/vds_node file upload -l root -p 123qwe -f ${DIR}/build/app/vds_node/vds_node -r ${DIR}/build/clients/2 -ll trace -lm "*"

echo "downloading file"
echo "build/app/vds_node/vds_node file download -l root -p 123qwe -f ${DIR}/build/vds_node.new -n vds_node -r ${DIR}/build/clients/1"
build/app/vds_node/vds_node file download -l root -p 123qwe -f ${DIR}/build/vds_node.new -n vds_node -r ${DIR}/build/clients/1 -ll trace -lm "*"

#fc /b %root_folder%vds.log %root_folder%vds.log.new