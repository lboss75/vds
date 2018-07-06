#!/bin/bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

~/bin/s3cmd-master/s3cmd sync --access_key=vadim@iv-soft.ru --secret_key=123 --no-ssl --host=localhost:8051 --host-bucket=localhost:8051 -v -r www s3://test/www_bucket/testpath