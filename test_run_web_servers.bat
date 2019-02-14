setlocal enableextensions enabledelayedexpansion

set root_folder=%~d0%~p0
rmdir %root_folder%servers /s /q

echo creating server
build\app\vds_background\Debug\vds_background.exe server root -dev -l test@iv-soft.ru -p 123qwe --root-folder C:\Users\v.malyshev\source\repos\vds\servers\0 -ll trace -lm *

set /A port=8050

for /L %%i in (0,1,10) do (
start build\app\vds_web_server\Debug\vds_web_server.exe server start -dev -P !port! --root-folder C:\Users\v.malyshev\source\repos\vds\servers\%%i -ll trace -lm * --web C:\Users\v.malyshev\source\repos\vds\www 
set /A port +=1
)