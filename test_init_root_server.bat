set root_folder=%~d0%~p0

rmdir %root_folder%servers /s /q

echo creating server
rem build\app\vds_background\Debug\vds_background.exe server root -l root -p 123qwe --root-folder %root_folder%servers\0 -ll trace -lm *
build\app\vds_background\Debug\vds_background.exe server root -l vadim@iv-soft.ru -p 123 --root-folder C:\Users\v.malyshev\source\repos\vds\servers\1 -ll trace -lm *