set root_folder=%~d0%~p0
rmdir %root_folder%\build_gtest /s /q

rem git clone https://github.com/google/googletest.git
call ..\setenv.bat

mkdir %root_folder%\build_gtest
cd %root_folder%\build_gtest

rem cmake -G "Visual Studio 15 2017 Win64" -Dgtest_force_shared_crt=ON %root_folder%\googletest\googletest
cmake -G %project_style% -Dgtest_force_shared_crt=ON %root_folder%\googletest\googletest

cmake --build .
