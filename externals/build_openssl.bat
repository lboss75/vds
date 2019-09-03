set root_folder=%~d0%~p0
rmdir %root_folder%\build_openssl /s /q

cd %root_folder%
git clone --depth=1 --single-branch --branch OpenSSL_1_1_1-stable --quiet https://github.com/openssl/openssl.git

set PATH=%PATH%;%HOMEDRIVE%%HOMEPATH%\AppData\Local\bin\NASM\;
call %root_folder%..\setenv.bat

mkdir %root_folder%\build_openssl
cd %root_folder%\openssl

if %project_style% == "Visual Studio 15 2017" goto x86
perl Configure debug-VC-WIN64A no-shared no-tests --prefix=%root_folder%\build_openssl --openssldir=%root_folder%\build_openssl
goto build

:x86
perl Configure debug-VC-WIN32 no-shared no-tests --prefix=%root_folder%\build_openssl --openssldir=%root_folder%\build_openssl

:build
nmake -f makefile clean
nmake -f makefile
nmake -f makefile test
nmake -f makefile install
