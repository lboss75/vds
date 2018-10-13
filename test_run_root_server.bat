set root_folder=%~d0%~p0

echo creating server
rem build\app\vds_background\Debug\vds_background.exe server root -l root -p 123qwe --root-folder %root_folder%servers\0 -ll trace -lm *
build\app\vds_background\Debug\vds_background.exe server start -P 8050 --root-folder C:\Users\v.malyshev\source\repos\vds\servers\0 -ll trace -lm *