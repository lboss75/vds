Virtual Distributed System

It is fully distributed storage system.

Build OpenSSL Android

export ANDROID_NDK=/home/vadim/Android/Sdk/ndk-bundle
PATH=$ANDROID_NDK/toolchains/arm-linux-androideabi-4.9/prebuilt/linux-x86_64:$ANDROID_NDK/toolchains/llvm/prebuilt/linux-x86_64/bin:$PATH
./Configure android-x86_64 no-tests no-shared no-ssl3 no-comp no-hw no-engine no-stdio no-ui-console  -D__ANDROID_API__=28 --prefix=/home/vadim/projects/openssl/build


PATH=$ANDROID_NDK/toolchains/x86_64-4.9/prebuilt/linux-x86_64:$ANDROID_NDK/toolchains/llvm/prebuilt/linux-x86_64/bin:$PATH
./Configure android-x86_64 no-tests no-shared no-ssl3 no-comp no-hw no-engine no-stdio no-ui-console   -D__ANDROID_API__=28 --prefix=/home/vadim/projects/openssl/build
