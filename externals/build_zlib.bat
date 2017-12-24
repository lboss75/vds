set root_folder=%~d0%~p0
rmdir %root_folder%build_zlib /s /q

call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\VC\Auxiliary\Build\vcvarsall.bat" x64

mkdir %root_folder%build_zlib
cd %root_folder%build_zlib

cmake -DCMAKE_INSTALL_PREFIX=%root_folder%zlib_out\ -G "Visual Studio 15 2017 Win64" %root_folder%zlib\
cmake --build .
cmake --build . --target install

