Virtual Distributed System

It is fully distributed storage system.
===================================================
Build Linux

------ build libC++

sudo add-apt-repository universe
sudo apt-get install -y git cmake clang-6.0 subversion libssl-dev zlib1g-dev

svn co http://llvm.org/svn/llvm-project/llvm/trunk llvm
cd llvm/projects                                                   
svn co http://llvm.org/svn/llvm-project/libcxx/trunk libcxx
svn co http://llvm.org/svn/llvm-project/libcxxabi/trunk libcxxabi
cd ..

export CC=/usr/bin/clang-6.0
export CXX=/usr/bin/clang++-6.0


mkdir build
cd build
cmake ..

make cxx
make install-cxx install-cxxabi

------ build GTest
git clone https://github.com/google/googletest.git gtest

export CC="/usr/bin/clang-6.0 -fPIC -fcoroutines-ts -pthread"
export CXX="/usr/bin/clang-6.0 -std=c++17 -fPIC -fcoroutines-ts -stdlib=libc++ -pthread"

cd gtest
mkdir build
cd build
cmake ..
make
sudo make install
------ buid VDS
mkdir build
cd build
cmake ..
make

 

=======
cd <vds>
mkdir build
cd build
cmake ..
make
sudo make install


===================================================
Build OpenSSL Android

export ANDROID_NDK=/home/vadim/Android/Sdk/ndk-bundle
PATH=$ANDROID_NDK/toolchains/arm-linux-androideabi-4.9/prebuilt/linux-x86_64:$ANDROID_NDK/toolchains/llvm/prebuilt/linux-x86_64/bin:$PATH
./Configure android-x86_64 no-tests no-shared no-ssl3 no-comp no-hw no-engine no-stdio no-ui-console  -D__ANDROID_API__=28 --prefix=/home/vadim/projects/openssl/build


PATH=$ANDROID_NDK/toolchains/x86_64-4.9/prebuilt/linux-x86_64:$ANDROID_NDK/toolchains/llvm/prebuilt/linux-x86_64/bin:$PATH
./Configure android-x86_64 no-tests no-shared no-ssl3 no-comp no-hw no-engine no-stdio no-ui-console   -D__ANDROID_API__=28 --prefix=/home/vadim/projects/openssl/build
-----------
RASPBERRY PI 3
Ubuntu MATE
sudo systemctl enable ssh
sudo service ssh restart

Install dependencies:
sudo apt-get update
sudo apt-get install -y git install cmake build-essential libssl-dev zlib1g-dev libgtest-dev 
sudo apt-get install -y git install cmake clang-6.0 libssl-dev zlib1g-dev libgtest-dev libc++-dev libc++abi-dev

cd /usr/src/gtest
sudo cmake CMakeLists.txt
sudo make

mkdir ~/projects
cd ~/projects
git clone https://github.com/lboss75/vds.git
cd ~/projects/vds

mkdir build
cd build
cmake .. -DGTEST_LIBRARY=/usr/src/gtest/libgtest.a -DGTEST_MAIN_LIBRARY=/usr/src/gtest/libgtest_main.a
make vds_web_server
cd app/vds_web_server
./vds_web_server server start --root-folder ~/projects/vds/build/app/vds_web_server  --web ~/projects/vds/www/

================
Co routenes
sudo apt-get install -y git install cmake clang-6.0 libssl-dev zlib1g-dev libgtest-dev libc++-dev

export CC=/usr/bin/clang-6.0
export CXX=/usr/bin/clang++-6.0
                                                   
svn co http://llvm.org/svn/llvm-project/libcxx/trunk libcxx
cd libcxx
mkdir build
cd build
cmake ..
make
sudo make install
