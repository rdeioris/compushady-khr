# Compushady C library exposing KHR GLSL/SPIRV features

## Build

For Windows, Linux and Mac

```sh
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

For Android:

(Change ~ with the directory containing the NDK)

```sh
mkdir build
cd build
cmake .. -DCMAKE_CXX_COMPILER=~/android-ndk-r27/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android28-clang++ -DCMAKE_C_COMPILER=~/android-ndk-r27/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android28-clang
cmake --build . --config Release
```

(On UNIX Systems remember to strip the .so!)
