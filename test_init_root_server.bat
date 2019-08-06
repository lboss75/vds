set root_folder=%~d0%~p0

rem rmdir %root_folder%servers /s /q

echo creating server
rem build\app\vds_background\Debug\vds_background.exe server root -l root -p 123qwe --root-folder %root_folder%servers\0 -ll trace -lm *
build\app\vds_background\Debug\vds_background.exe server root -l vadim@iv-soft.ru -p 123qwe