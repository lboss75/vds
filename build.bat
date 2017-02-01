rmdir build /s /q
mkdir build
cd build
cmake.exe  -DZLIB_INCLUDE_DIR=D:\projects\zlib -DZLIB_LIBRARY=D:\projects\zlib ../ -G "Visual Studio 14 2015 Win64"