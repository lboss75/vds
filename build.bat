set root_folder=%~d0%~p0

call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\Common7\Tools\VsDevCmd.bat" 

del %root_folder%\CMakeSettings.json
rmdir %root_folder%\build /s /q
mkdir %root_folder%\build
cd %root_folder%\build

cmake.exe  -DZLIB_INCLUDE_DIR=%root_folder%\externals\zlib_out\include -DZLIB_LIBRARY=%root_folder%\externals\zlib_out\lib\zlibstaticd.lib -DOPENSSL_ROOT_DIR=%root_folder%\externals\Build-VC-64-debug\ -DGTEST_LIBRARY=%root_folder%\externals\build_gtest\Debug\gtestd.lib -DGTEST_INCLUDE_DIR=%root_folder%\externals\googletest\googletest\include -DGTEST_MAIN_LIBRARY=%root_folder%\externals\build_gtest\Debug\gtest_maind.lib ../ -G "Visual Studio 15 2017 Win64"
rem cmake.exe  -DZLIB_INCLUDE_DIR=D:\projects\zlib -DZLIB_LIBRARY=D:\projects\zlib\zlib.lib ../ -G "Visual Studio 15 2017 Win64"

start vds.sln 