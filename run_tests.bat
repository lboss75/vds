set root_folder=%~d0%~p0

rmdir %root_folder%servers /s /q

echo creating server
build\app\vds_background\Debug\vds_background.exe server root -l vadim@iv-soft.ru -p 123 --root-folder C:\Users\v.malyshev\source\repos\vds\servers\4 -ll trace -lm dht_sync

start build\app\vds_web_server\Debug\vds_web_server.exe server start -P 8050 --root-folder C:\Users\v.malyshev\source\repos\vds\servers\0 -ll trace -lm dht_sync --web C:\Users\v.malyshev\source\repos\vds\www
rem start build\app\vds_web_server\Debug\vds_web_server.exe server start -P 8051 --root-folder C:\Users\v.malyshev\source\repos\vds\servers\1 -ll trace -lm dht_sync --web C:\Users\v.malyshev\source\repos\vds\www
start build\app\vds_web_server\Debug\vds_web_server.exe server start -P 8052 --root-folder C:\Users\v.malyshev\source\repos\vds\servers\2 -ll trace -lm dht_sync --web C:\Users\v.malyshev\source\repos\vds\www
start build\app\vds_web_server\Debug\vds_web_server.exe server start -P 8053 --root-folder C:\Users\v.malyshev\source\repos\vds\servers\3 -ll trace -lm dht_sync --web C:\Users\v.malyshev\source\repos\vds\www
start build\app\vds_web_server\Debug\vds_web_server.exe server start -P 8054 --root-folder C:\Users\v.malyshev\source\repos\vds\servers\4 -ll trace -lm dht_sync --web C:\Users\v.malyshev\source\repos\vds\www