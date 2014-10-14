#!/bin/bash
#export TMPDIR=/cygdrive/f/android-project/temp

NDKROOT=/cygdrive/f/android/android-ndk-r4

#PREBUILT=$NDKROOT/toolchains/arm-linux-androideabi-4.4.3/prebuilt/windows
PREBUILT=$NDKROOT/build/prebuilt/windows/arm-eabi-4.4.0
PLATFORM=$NDKROOT/build/platforms/android-8/arch-arm


./configure --target-os=linux \
	--arch=arm \
	--enable-version3 \
	--enable-gpl \
	--enable-nonfree \
	--disable-stripping \
	--disable-ffmpeg \
	--disable-ffplay \
	--disable-ffserver \
	--disable-ffprobe \
	--disable-encoders \
	--disable-muxers \
	--disable-devices \
	--disable-protocols \
	--enable-protocol=file \
	--enable-protocol=HTTP \
	--enable-avfilter \
	--enable-network \
	--disable-avdevice \
	--enable-cross-compile \
	--cc=$PREBUILT/bin/arm-eabi-gcc \
	--cross-prefix=$PREBUILT/bin/arm-eabi- \
	--nm=$PREBUILT/bin/arm-eabi-nm \
	--extra-cflags="-fPIC -DANDROID" \
	--disable-asm \
	--enable-neon \
	--enable-armv5te \
	--extra-ldflags="-Wl,-T,$PREBUILT/arm-eabi/lib/ldscripts/armelf.x -Wl,-rpath-link=$PLATFORM/usr/lib -L$PLATFORM/usr/lib -nostdlib $PREBUILT/lib/gcc/arm-eabi/4.4.0/crtbegin.o $PREBUILT/lib/gcc/arm-eabi/4.4.0/crtend.o -lc -lm -ldl"