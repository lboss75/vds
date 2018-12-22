set root_folder=%~d0%~p0
rmdir %root_folder%build_zlib /s /q

call ..\setenv.bat

mkdir %root_folder%build_zlib
cd %root_folder%build_zlib

rem cmake -DCMAKE_INSTALL_PREFIX=%root_folder%zlib_out\ -G "Visual Studio 15 2017 Win64" %root_folder%zlib\
cmake -DCMAKE_INSTALL_PREFIX=%root_folder%zlib_out\ -G %project_style% %root_folder%zlib\

cmake --build .
cmake --build . --target install

