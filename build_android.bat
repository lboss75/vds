set ARCH=x86_64
set root_folder=%~d0%~p0

set ANDROID_NDK=C:\Microsoft\AndroidNDK64\android-ndk-r18b
set CLANG_NDK=%ANDROID_NDK%\toolchains\llvm\prebuilt\windows-x86_64\bin\clang.exe
set CXXLANG_NDK=%ANDROID_NDK%\toolchains\llvm\prebuilt\windows-x86_64\bin\clang++.exe
set MAKE_NDK=%ANDROID_NDK%\prebuilt\windows-x86_64\bin\make.exe 

goto cont

rmdir %root_folder%\build-android /s /q
mkdir %root_folder%\build-android
cd %root_folder%\build-android

mkdir externals
cd externals

git clone --quiet https://github.com/openssl/openssl.git

rmdir openssl-out /s /q
mkdir openssl-out

cd openssl

:cont
set PATH=%ANDROID_NDK%\toolchains\llvm\prebuilt\windows-x86_64\bin;%PATH%

# Tell configure what tools to use.
set AS=clang.exe
set CC=clang.exe
set CXX=clang++.exe
set LD=clang.exe

perl Configure android-x86_64 no-tests -fno-integrated-as --prefix=%root_folder%\build-android\externals\openssl-out --openssldir=%root_folder%\build-android\externals\openssl-out

goto end

make
makeinstall
cd ..


rem cmake .. -DCMAKE_TOOLCHAIN_FILE=%NDK%\build\cmake\android.toolchain.cmake -DCMAKE_SYSTEM_NAME="Android" -DANDROID_NDK=%NDK% -DANDROID_TOOLCHAIN=clang -DANDROID_PLATFORM=android-24 -DCMAKE_MAKE_PROGRAM=%MAKE_NDK% -G "Unix Makefiles"


goto end

rm -rf openssl-out
set -e

cd openssl
./Configure linux-x86_64-clang no-tests --prefix=$PWD/../openssl-out --openssldir=$PWD/../openssl-out
make --quiet
make --quiet install
cd ..

set +e
git clone --quiet https://github.com/madler/zlib.git
rm -rf zlib_out
set -e

mkdir zlib_out
cd zlib_out
cmake ../zlib
make --quiet
make --quiet install
cd ..

set +e
git clone --quiet https://github.com/google/googletest.git
rm -rf gtest_out
set -e

mkdir gtest_out
cd gtest_out
cmake -Dgtest_force_shared_crt=ON ../googletest/googletest
make --quiet
cd ..\..
rm -rf build
mkdir build
cd build
cmake -DZLIB_INCLUDE_DIR=$PWD/../externals/zlib_out/include -DZLIB_LIBRARY=$PWD/../externals/zlib_out/libz.a -DOPENSSL_ROOT_DIR=$PWD/../externals/openssl-out/ -DGTEST_LIBRARY=$PWD/../externals/gtest_out/lib/libgtest.a -DGTEST_INCLUDE_DIR=$PWD/../externals/googletest/googletest/include -DGTEST_MAIN_LIBRARY=$PWD/../externals/gtest_out/lib/libgtest_main.a .. 
make --quiet

cd app/vds_web_server
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib/
./vds_web_server server service --root-folder ${DIR}/build/app/vds_web_server  --web ${DIR}/www/ -lm \* -ll trace




rem cmake -DCMAKE_C_COMPILER=%CLANG_NDK% -DCMAKE_CXX_COMPILER=%CXXLANG_NDK% ../ -G "NMake Makefiles"
cmake .. -DCMAKE_TOOLCHAIN_FILE=%NDK%\build\cmake\android.toolchain.cmake -DCMAKE_SYSTEM_NAME="Android" -DANDROID_NDK=%NDK% -DANDROID_TOOLCHAIN=clang -DANDROID_PLATFORM=android-24 -DCMAKE_MAKE_PROGRAM=%MAKE_NDK% -G "Unix Makefiles"


:end
