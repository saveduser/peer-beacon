mkdir -p build/android/arm64-v8a
mkdir -p build/android/armeabi-v7a
mkdir -p build/android/x86_64
export PATH=$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/linux-x86_64/bin:$PATH

SRC_FILES="src/*.cpp src/platform/*.cpp"

aarch64-linux-android21-clang++ \
    -shared \
    -fPIC \
    -std=c++17 \
    -Isrc \
    $SRC_FILES \
    -o build/android/arm64-v8a/libpeerbeacon-arm64.so
armv7a-linux-androideabi21-clang++ \
    -shared \
    -fPIC \
    -std=c++17 \
    -Isrc \
    $SRC_FILES \
    -o build/android/armeabi-v7a/libpeerbeacon-arm32.so

x86_64-linux-android21-clang++ \
    -shared \
    -fPIC \
    -std=c++17 \
    -Isrc \
    $SRC_FILES \
    -o build/android/x86_64/libpeerbeacon-x86_64.so