set root_folder=%~d0%~p0
rmdir %root_folder%\Build-VC-64-debug /s /q

set PATH=%PATH%;%HOMEDRIVE%%HOMEPATH%\AppData\Local\bin\NASM\;
call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\VC\Auxiliary\Build\vcvarsall.bat" x64

mkdir %root_folder%\Build-VC-64-debug
cd %root_folder%\openssl
perl Configure debug-VC-WIN64A --prefix=%root_folder%\Build-VC-64-debug --openssldir=%root_folder%\Build-VC-64-debug

nmake -f makefile clean
nmake -f makefile
nmake -f makefile test
nmake -f makefile install
