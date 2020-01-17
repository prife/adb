# arm_adb

Android's adb standalone build with cmake, support x86/x64 and arm/arm64(aarch64)

It's recommend to develop adb with clion which remote build is so useful!

## Prerequisites

### install toolchain

Please prepare an linux environment.

for arm/arm64, please install toolchain, for example you can download from 
- https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-a/downloads

git clone this project.

```bash
$ cd <this-project>
$ cd lib
$ git clone https://salsa.debian.org/android-tools-team/android-platform-external-boringssl.git boringssl
```

### build boringssl

#### ARM64(AARCH64)

```bash
$ cd boringssl
$ rm -rf debian/out
$ make CFLAGS=-fPIC CC=aarch64-linux-gnu-gcc DEB_HOST_ARCH=arm64 -f debian/libcrypto.mk
$ make CFLAGS=-fPIC CXX=aarch64-linux-gnu-g++ DEB_HOST_ARCH=arm64 -f debian/libssl.mk
```

#### ARM

```bash
$ cd boringssl
$ make CFLAGS=-fPIC CC=arm-linux-gnu-gcc DEB_HOST_ARCH=armel -f debian/libcrypto.mk
$ make CFLAGS=-fPIC CXX=arm-linux-gnu-g++ DEB_HOST_ARCH=armel -f debian/libssl.mk
```

#### X64(AMD64)

```bash
$ cd boringssl
$ rm -rf debian/out
$ make CFLAGS=-fPIC DEB_HOST_ARCH=amd64 -f debian/libcrypto.mk
$ make CFLAGS=-fPIC DEB_HOST_ARCH=amd64 -f debian/libssl.mk
```

## build adb

build with Clion please, so easy!

**NOTE: not support windows/mac yet!"

## Troubleshooting
