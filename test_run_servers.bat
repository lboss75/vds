set root_folder=%~d0%~p0

rmdir %root_folder%servers /s /q

echo creating server
build\app\vds_background\Debug\vds_background.exe server root -l vadim@iv-soft.ru -p 123qwe --root-folder %root_folder%servers\0 -ll trace -lm *
start build\app\vds_web_server\Debug\vds_web_server.exe server start -P 8050 --root-folder %root_folder%servers\0 -ll trace -lm *
start build\app\vds_web_server\Debug\vds_web_server.exe server start -P 8051 --root-folder %root_folder%servers\1 -ll trace -lm *
start build\app\vds_web_server\Debug\vds_web_server.exe server start -P 8052 --root-folder %root_folder%servers\2 -ll trace -lm *
start build\app\vds_web_server\Debug\vds_web_server.exe server start -P 8053 --root-folder %root_folder%servers\3 -ll trace -lm *
start build\app\vds_web_server\Debug\vds_web_server.exe server start -P 8054 --root-folder %root_folder%servers\4 -ll trace -lm *
start build\app\vds_web_server\Debug\vds_web_server.exe server start -P 8055 --root-folder %root_folder%servers\5 -ll trace -lm *
start build\app\vds_web_server\Debug\vds_web_server.exe server start -P 8056 --root-folder %root_folder%servers\6 -ll trace -lm *
start build\app\vds_web_server\Debug\vds_web_server.exe server start -P 8057 --root-folder %root_folder%servers\7 -ll trace -lm *
start build\app\vds_web_server\Debug\vds_web_server.exe server start -P 8058 --root-folder %root_folder%servers\8 -ll trace -lm *
start build\app\vds_web_server\Debug\vds_web_server.exe server start -P 8059 --root-folder %root_folder%servers\9 -ll trace -lm *

mkdir %root_folder%servers\0\storage
mkdir %root_folder%servers\1\storage
mkdir %root_folder%servers\2\storage
mkdir %root_folder%servers\3\storage
mkdir %root_folder%servers\4\storage
mkdir %root_folder%servers\5\storage
mkdir %root_folder%servers\6\storage
mkdir %root_folder%servers\7\storage
mkdir %root_folder%servers\8\storage
mkdir %root_folder%servers\9\storage


build\app\vds_cmd\Debug\vds_cmd.exe storage add -l vadim@iv-soft.ru -p 123qwe --storage-path %root_folder%servers\0\storage --storage-name default --storage-size 4294967296 -s tcp://localhost:8050
build\app\vds_cmd\Debug\vds_cmd.exe storage add -l vadim@iv-soft.ru -p 123qwe --storage-path %root_folder%servers\1\storage --storage-name default --storage-size 4294967296 -s tcp://localhost:8051
build\app\vds_cmd\Debug\vds_cmd.exe storage add -l vadim@iv-soft.ru -p 123qwe --storage-path %root_folder%servers\2\storage --storage-name default --storage-size 4294967296 -s tcp://localhost:8052
build\app\vds_cmd\Debug\vds_cmd.exe storage add -l vadim@iv-soft.ru -p 123qwe --storage-path %root_folder%servers\3\storage --storage-name default --storage-size 4294967296 -s tcp://localhost:8053
build\app\vds_cmd\Debug\vds_cmd.exe storage add -l vadim@iv-soft.ru -p 123qwe --storage-path %root_folder%servers\4\storage --storage-name default --storage-size 4294967296 -s tcp://localhost:8054
build\app\vds_cmd\Debug\vds_cmd.exe storage add -l vadim@iv-soft.ru -p 123qwe --storage-path %root_folder%servers\5\storage --storage-name default --storage-size 4294967296 -s tcp://localhost:8055
build\app\vds_cmd\Debug\vds_cmd.exe storage add -l vadim@iv-soft.ru -p 123qwe --storage-path %root_folder%servers\6\storage --storage-name default --storage-size 4294967296 -s tcp://localhost:8056
build\app\vds_cmd\Debug\vds_cmd.exe storage add -l vadim@iv-soft.ru -p 123qwe --storage-path %root_folder%servers\7\storage --storage-name default --storage-size 4294967296 -s tcp://localhost:8057
build\app\vds_cmd\Debug\vds_cmd.exe storage add -l vadim@iv-soft.ru -p 123qwe --storage-path %root_folder%servers\8\storage --storage-name default --storage-size 4294967296 -s tcp://localhost:8058
build\app\vds_cmd\Debug\vds_cmd.exe storage add -l vadim@iv-soft.ru -p 123qwe --storage-path %root_folder%servers\9\storage --storage-name default --storage-size 4294967296 -s tcp://localhost:8059

