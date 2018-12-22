set root_folder=%~d0%~p0
rmdir %root_folder%\build_openssl /s /q

set PATH=%PATH%;%HOMEDRIVE%%HOMEPATH%\AppData\Local\bin\NASM\;
call ..\setenv.bat

mkdir %root_folder%\build_openssl
cd %root_folder%\openssl
rem perl Configure debug-VC-WIN64A no-shared no-tests --prefix=%root_folder%\build_openssl --openssldir=%root_folder%\build_openssl
perl Configure debug-VC-WIN32 no-shared no-tests --prefix=%root_folder%\build_openssl --openssldir=%root_folder%\build_openssl

nmake -f makefile clean
nmake -f makefile
nmake -f makefile test
nmake -f makefile install
