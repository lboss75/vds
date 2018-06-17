set root_folder=%~d0%~p0
rmdir %root_folder%\android64 /s /q

set PATH=%PATH%;%HOMEDRIVE%%HOMEPATH%\AppData\Local\bin\NASM\;

mkdir %root_folder%\android64
cd %root_folder%\openssl

set ANDROID_NDK=C:\Users\v.malyshev\AppData\Local\Android\Sdk\ndk-bundle
set PATH=%ANDROID_NDK%\toolchains\x86-4.9\prebuilt\windows-x86_64\bin;%PATH%;
perl Configure android-x86 no-shared no-tests --prefix=%root_folder%\android64 --openssldir=%root_folder%\android64 -D__ANDROID_API__=14

make clean
make 
make test
make install
