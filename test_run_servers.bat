set root_folder=%~d0%~p0

rmdir %root_folder%servers /s /q

echo creating server
build\app\vds_background\Debug\vds_background.exe server root -l vadim@iv-soft.ru -p 123 --root-folder C:\Users\v.malyshev\source\repos\vds\servers\0 -ll trace -lm *
start build\app\vds_web_server\Debug\vds_web_server.exe server start -P 8050 --root-folder C:\Users\v.malyshev\source\repos\vds\servers\0 -ll trace -lm * --web C:\Users\v.malyshev\source\repos\vds\www
start build\app\vds_web_server\Debug\vds_web_server.exe server start -P 8051 --root-folder C:\Users\v.malyshev\source\repos\vds\servers\1 -ll trace -lm * --web C:\Users\v.malyshev\source\repos\vds\www
start build\app\vds_web_server\Debug\vds_web_server.exe server start -P 8052 --root-folder C:\Users\v.malyshev\source\repos\vds\servers\2 -ll trace -lm * --web C:\Users\v.malyshev\source\repos\vds\www
start build\app\vds_web_server\Debug\vds_web_server.exe server start -P 8053 --root-folder C:\Users\v.malyshev\source\repos\vds\servers\3 -ll trace -lm * --web C:\Users\v.malyshev\source\repos\vds\www
start build\app\vds_web_server\Debug\vds_web_server.exe server start -P 8054 --root-folder C:\Users\v.malyshev\source\repos\vds\servers\4 -ll trace -lm * --web C:\Users\v.malyshev\source\repos\vds\www
start build\app\vds_web_server\Debug\vds_web_server.exe server start -P 8055 --root-folder C:\Users\v.malyshev\source\repos\vds\servers\5 -ll trace -lm * --web C:\Users\v.malyshev\source\repos\vds\www
start build\app\vds_web_server\Debug\vds_web_server.exe server start -P 8056 --root-folder C:\Users\v.malyshev\source\repos\vds\servers\6 -ll trace -lm * --web C:\Users\v.malyshev\source\repos\vds\www
start build\app\vds_web_server\Debug\vds_web_server.exe server start -P 8057 --root-folder C:\Users\v.malyshev\source\repos\vds\servers\7 -ll trace -lm * --web C:\Users\v.malyshev\source\repos\vds\www
start build\app\vds_web_server\Debug\vds_web_server.exe server start -P 8058 --root-folder C:\Users\v.malyshev\source\repos\vds\servers\8 -ll trace -lm * --web C:\Users\v.malyshev\source\repos\vds\www
start build\app\vds_web_server\Debug\vds_web_server.exe server start -P 8059 --root-folder C:\Users\v.malyshev\source\repos\vds\servers\9 -ll trace -lm * --web C:\Users\v.malyshev\source\repos\vds\www
