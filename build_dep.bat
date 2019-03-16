set this_folder=%~d0%~p0

cd %this_folder%externals
call build_gtest.bat

cd %this_folder%externals
call build_openssl.bat

cd %this_folder%externals
call build_zlib.bat

cd %this_folder%
