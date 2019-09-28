set root_folder=%~d0%~p0
rmdir %root_folder%build_zlib /s /q

git clone --depth=1 --quiet https://github.com/madler/zlib.git

call ..\setenv.bat

mkdir %root_folder%build_zlib
cd %root_folder%build_zlib

rem cmake -DCMAKE_INSTALL_PREFIX=%root_folder%zlib_out\ -G "Visual Studio 15 2017" %root_folder%zlib\
cmake -DCMAKE_INSTALL_PREFIX=%root_folder%zlib_out\ -G %project_style% -A %project_arch% %root_folder%zlib\

cmake --build .
cmake --build . --target install

