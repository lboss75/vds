set root_folder=%~d0%~p0
rmdir %root_folder%\build_gtest /s /q

rem git clone https://github.com/google/googletest.git
call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x64 8.1 

mkdir %root_folder%\build_gtest
cd %root_folder%\build_gtest

rem cmake -G "Visual Studio 15 2017 Win64" -Dgtest_force_shared_crt=ON %root_folder%\googletest\googletest
cmake -G "Visual Studio 14 2015 Win64" -Dgtest_force_shared_crt=ON %root_folder%\googletest\googletest

cmake --build .
