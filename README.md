# adb

Android's adb standalone build with cmake, support Linux(x86-64、arm64)], Windows(32bit) and macOS!

It's recommend to develop adb with clion/vscode which's remote development is so usefull!

## build adb target for Linux(X64、arm64)

### step1: install toolchain

Please prepare an linux environment.

for arm/arm64, please install toolchain, for example you can download from 
- https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-a/downloads

git clone this project.

```bash
$ cd <this-project>
$ cd lib
$ git clone https://salsa.debian.org/android-tools-team/android-platform-external-boringssl.git boringssl
$ cd boringssl
$ git checkout debian/8.1.0+r23-3
```

### step2: build boringssl

Please choose a platform as following. 

1. on intel pc with ubuntu Linux, please follow `X64(AMD64)`
2. on Raspberry Pi or RK3399 or RK3328 with ubuntu Linux, please follow `ARM64(AARCH64)`

**X86-64(AMD64)**

```bash
$ cd boringssl
$ rm -rf debian/out
$ make CFLAGS=-fPIC DEB_HOST_ARCH=amd64 -f debian/libcrypto.mk
$ make CXXFLAGS=-fPIC DEB_HOST_ARCH=amd64 -f debian/libssl.mk
```

**ARM64(AARCH64)**

```bash
$ cd boringssl
$ rm -rf debian/out
$ make CFLAGS=-fPIC CC=aarch64-linux-gnu-gcc DEB_HOST_ARCH=arm64 -f debian/libcrypto.mk
$ make CXXFLAGS=-fPIC CXX=aarch64-linux-gnu-g++ DEB_HOST_ARCH=arm64 -f debian/libssl.mk
```

### build adb for linux x86-64

**build on command line**

```bash
$ mkdir build && cd build
$ cmake ..
$ make -j8
```

**build with clion/vscode on windows**

you need an remote linux pc, then config clion/vscode with it's remote development feature, it is so easy!

### build adb target for linux aarch64

NOTE: please install a cmake with the newest version and `lld` !

```
sudo apt-get install -y lld
```

**first, install toolchain**

```bash
$ cd cmake/toolchain/linux-aarch64
$ wget 'https://developer.arm.com/-/media/Files/downloads/gnu-a/9.2-2019.12/binrel/gcc-arm-9.2-2019.12-x86_64-aarch64-none-linux-gnu.tar.xz\?revision\=61c3be5d-5175-4db6-9030-b565aae9f766\&la\=en\&hash\=0A37024B42028A9616F56A51C2D20755C5EBBCD7' -O gcc-arm-9.2-2019.12-x86_64-aarch64-none-linux-gnu.tar.xz
$ tar xvf gcc-arm-9.2-2019.12-x86_64-aarch64-none-linux-gnu.tar.xz --strip-components=1
$ cd -
```

**build with cmake**

```
$ CC=clang CXX=clang++ cmake . -Bbuild-arm64 -DCMAKE_TOOLCHAIN_FILE=cmake/linux/toolchain-aarch64.cmake
$ cmake --build build-arm64 --config Release
```

ref: https://clickhouse.tech/docs/en/development/build-cross-arm/

## build adb target for windows(only 32-bit supported!)

1. install msys2

config repo with https://mirror.tuna.tsinghua.edu.cn/help/msys2/

```
$ pacman -S mingw-w64-i686-gcc
$ pacman -S mingw-w64-i686-cmake
$ pacman -S make
```

2. download source

```
$ cd lib
$ git clone https://salsa.debian.org/android-tools-team/android-platform-external-boringssl.git boringssl
$ cd ..
```

3. build boringssl

It's not necessary to build boringssl, because I've already prebuilt on `prebuilt/windows/32/libcrypto.a`

If you really want build by yourself. Please

```
$ cd lib/boringssl
$ cp ../prebuilt/CMakeLists_boringssl.txt CMakeLists.txt
$ mkdir build32 && cd build32
$ cmake -G"Unix Makefiles" ..
$ make -j8
```

If there is nothing wrong. the `libcrypto.a` will be built out.

cp it to `n-adb/prebuilt/windows/32/`

4. build adb

switch to the n-adb source direcoty, and run following commands

```
$ mkdir build32
$ cd build32
$ cmake -G"Unix Makefiles" ..
$ make -j8
```

## build adb target for macOS

1. download source

```
$ cd lib
$ git clone https://salsa.debian.org/android-tools-team/android-platform-external-boringssl.git boringssl
$ cd ..
```

2. build boringssl

It's not necessary to build boringssl, because I've already prebuilt on `prebuilt/osx/libcrypto.a`

If you really want build by yourself. Please

```
$ cd lib/boringssl
$ cp ../prebuilt/CMakeLists_boringssl.txt CMakeLists.txt
$ mkdir build32 && cd build32
$ cmake ..
$ make -j8
```

If there is nothing wrong. the `libcrypto.a` will be built out.

cp it to `n-adb/prebuilt/osx/`

3. build adb

```bash
$ mkdir build && cd build
$ cmake ..
$ make -j8
```

## Troubleshooting
