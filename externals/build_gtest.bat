set root_folder=%~d0%~p0
rmdir %root_folder%\build_gtest /s /q

rem git clone https://github.com/google/googletest.git

mkdir %root_folder%\build_gtest
cd %root_folder%\build_gtest

cmake -G "Visual Studio 15 2017 Win64" %root_folder%\googletest\googletest
cmake --build .
