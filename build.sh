#!/bin/bash
DL="https://github.com/openipc/firmware/releases/download/latest"
SUFFIX=tgz
TAR="tar -xf"
ARCH=arm-linux

if [[ "$1" == *"star6b0" ]]; then
	CC=cortex_a7_thumb2_hf-gcc13-musl-4_9
elif [[ "$1" == *"star6e" ]]; then
	CC=cortex_a7_thumb2_hf-gcc13-glibc-4_9
elif [[ "$1" == *"goke" ]]; then
	CC=cortex_a7_thumb2-gcc13-musl-4_9
elif [[ "$1" == *"hisi" ]]; then
	CC=cortex_a7_thumb2-gcc13-musl-4_9
elif [[ "$1" == *"rockchip" ]]; then
	CC=gcc-linaro-7.3.1-2018.05-x86_64_aarch64-linux-gnu
	SUFFIX=tar.xz
	TAR="tar -Jxf"
	ARCH=aarch64-linux-gnu
	DL="https://releases.linaro.org/components/toolchain/binaries/7.3-2018.05/${ARCH}"
fi

GCC=$PWD/toolchain/$CC/bin/$ARCH-gcc
OUT=wfb_bind_$1

if [ ! -e toolchain/$CC ]; then
	wget -c -q --show-progress $DL/$CC.$SUFFIX -P $PWD
	mkdir -p toolchain/$CC
	$TAR $CC.$SUFFIX -C toolchain/$CC --strip-components=1 || exit 1
	rm -f $CC.$SUFFIX
fi

if [ "$1" = "goke" ]; then
	make -B CC=$GCC TOOLCHAIN=$PWD/toolchain/$CC OUTPUT=$OUT $1
elif [ "$1" = "hisi" ]; then
	DRV=$PWD/firmware/general/package/hisilicon-osdrv-hi3516ev200/files/lib
	make -B CC=$GCC TOOLCHAIN=$PWD/toolchain/$CC OUTPUT=$OUT $1
elif [ "$1" = "star6b0" ]; then
	DRV=$PWD/firmware/general/package/sigmastar-osdrv-infinity6b0/files/lib
	make -B CC=$GCC  TOOLCHAIN=$PWD/toolchain/$CC OUTPUT=$OUT $1
elif [ "$1" = "star6e" ]; then
	DRV=$PWD/firmware/general/package/sigmastar-osdrv-infinity6e/files/lib
	make -B CC=$GCC TOOLCHAIN=$PWD/toolchain/$CC OUTPUT=$OUT $1
elif [ "$1" = "x86" ]; then
	make OUTPUT=$OUT $1
elif [ "$1" = "rockchip" ]; then
	make CC=$GCC TOOLCHAIN=$PWD/toolchain/$CC OUTPUT=$OUT $1
else
	echo "Usage: $0 [goke|hisi|star6b0|star6e|rockchip]"
	exit 1
fi
