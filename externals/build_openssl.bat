set root_folder=%~d0%~p0
rmdir %root_folder%\Build-VC-64-debug /s /q

set PATH=%PATH%;%HOMEDRIVE%%HOMEPATH%\AppData\Local\bin\NASM\;
call ..\setenv.bat

mkdir %root_folder%\Build-VC-64-debug
cd %root_folder%\openssl
perl Configure debug-VC-WIN64A no-shared no-tests --prefix=%root_folder%\Build-VC-64-debug --openssldir=%root_folder%\Build-VC-64-debug

nmake -f makefile clean
nmake -f makefile
nmake -f makefile test
nmake -f makefile install
