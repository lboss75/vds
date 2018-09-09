Virtual Distributed System

It is fully distributed storage system.

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
sudo apt-get install -y git install cmake clang-5.0 libssl-dev zlib1g-dev libgtest-dev libc++-dev

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
make
