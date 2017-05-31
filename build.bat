rmdir build /s /q
mkdir build
cd build

cmake.exe  -DZLIB_INCLUDE_DIR=D:\projects\zlib -DZLIB_LIBRARY=D:\projects\zlib\zlib.lib ../ -G "Visual Studio 14 2015 Win64"
rem cmake.exe  -DZLIB_INCLUDE_DIR=D:\projects\zlib -DZLIB_LIBRARY=D:\projects\zlib\zlib.lib ../ -G "Visual Studio 15 2017 Win64"

start vds.sln 