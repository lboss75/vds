set root_folder=%~d0%~p0
rmdir %root_folder%\build_gtest /s /q

git clone --depth=1 https://github.com/google/googletest.git
call %root_folder%..\setenv.bat

mkdir %root_folder%\build_gtest
cd %root_folder%\build_gtest

cmake -G %project_style% -A %project_arch% -Dgtest_force_shared_crt=ON %root_folder%\googletest\googletest

cmake --build .
