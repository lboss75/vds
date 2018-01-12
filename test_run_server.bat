set root_folder=%~d0%~p0
set index=%1
set port=%2

rem rmdir %root_folder%servers\%index% /s /q

echo start server
build\app\vds_background\Debug\vds_background.exe server start -P %port% --root-folder %root_folder%servers\%index% -ll trace -lm *

