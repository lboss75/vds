#!/bin/bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

./vds_web_server.exe server start -P 8050 --root-folder $DIR/server -ll trace -lm * --web $DIR/www
