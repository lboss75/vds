set root_folder=%~d0%~p0

build\app\vds_web_server\Debug\vds_web_server.exe server start -P 8050 -ll trace -lm * --web %root_folder%www 