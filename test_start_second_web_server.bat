set root_folder=%~d0%~p0

rem build\app\vds_background\Debug\vds_background.exe server root -l root -p 123qwe --root-folder %root_folder%servers\0 -ll trace -lm *
build\app\vds_web_server\Debug\vds_web_server.exe server start -P 8052 --root-folder C:\Users\v.malyshev\source\repos\vds\servers\2 -ll trace -lm DHT_SYNC --web C:\Users\v.malyshev\source\repos\vds\www
