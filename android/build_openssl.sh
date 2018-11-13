export ANDROID_NDK=/home/vadim/Android/Sdk/ndk-bundle
PATH=$ANDROID_NDK/toolchains/arm-linux-androideabi-4.9/prebuilt/linux-x86:$ANDROID_NDK/toolchains/llvm/prebuilt/linux-x86/bin:$PATH
cd ../../openssl
./Configure android-x86 no-shared no-asm no-ssl3 no-comp no-hw no-engine no-stdio -D__ANDROID_API__=28 --prefix=/home/vadim/projects/openssl/build
make
