
set -x -e

cd FFmpeg

if [ "$1" == "arm" ]; then
  echo "Compiling for ARM"
  PLATFORM_FLAGS=(--target-os=win32 --as=armasm --arch=arm --cpu=armv7  --enable-cross-compile --toolchain=msvc --extra-cflags="-MD -GL -D__ARM_PCS_VFP" --extra-cxxflags="-MD -GL" --extra-ldflags="-nodefaultlib:LIBCMT -LTCG")
  PLATFORM=win
elif [ "$1" == "arm64" ]; then
  echo "Compiling for ARM64"
  PLATFORM_FLAGS=(--target-os=win64 --as=armasm64 --arch=aarch64 --enable-cross-compile --toolchain=msvc --extra-cflags="-MD -GL" --extra-cxxflags="-MD -GL" --extra-ldflags="-nodefaultlib:LIBCMT -LTCG")
  PLATFORM=win
elif [ "$1" == "linux-arm64" ]; then
  echo "Compiling for Linux ARM64"
  PLATFORM_FLAGS=(--target-os=linux --arch=aarch64 --enable-cross-compile --cross-prefix=aarch64-linux-gnu- --enable-pic --extra-cflags="-flto")
  PLATFORM=linux
  sudo apt install -y binutils-aarch64-linux-gnu gcc-aarch64-linux-gnu g++-aarch64-linux-gnu
elif [ "$1" == "linux-x64" ]; then
  echo "Compiling for Linux x64"
  PLATFORM_FLAGS=(--target-os=linux --arch=x64 --enable-x86asm)
  PLATFORM=linux
  sudo apt install -y binutils-aarch64-linux-gnu gcc-aarch64-linux-gnu g++-aarch64-linux-gnu
elif [ "$1" == "x86" ]; then
  echo "Compiling for x86"
  PLATFORM_FLAGS=(--target-os=win32 --enable-x86asm --arch=i686 --cpu=x86 --toolchain=msvc --extra-cflags="-MD -GL" --extra-cxxflags="-MD -GL" --extra-ldflags="-nodefaultlib:LIBCMT -LTCG")
  PLATFORM=win
elif [ "$1" == "x64" ]; then
  echo "Compiling for x64"
  PLATFORM_FLAGS=(--target-os=win64 --enable-x86asm --arch=amd64 --cpu=amd64 --toolchain=msvc --extra-cflags="-MD -GL" --extra-cxxflags="-MD -GL" --extra-ldflags="-nodefaultlib:LIBCMT -LTCG")
  PLATFORM=win
else
  echo "Pass arch argument"
  exit 1
fi

if [ "$PLATFORM" == "win" ]; then
  pacman --sync --noconfirm --needed base-devel
  pacman --sync --noconfirm --needed p7zip
  pacman --sync --noconfirm --needed zlib

  wget http://www.tortall.net/projects/yasm/releases/yasm-1.3.0-win64.exe -O /bin/yasm.exe
fi

# ffmpeg needs this script
wget https://raw.githubusercontent.com/FFmpeg/gas-preprocessor/master/gas-preprocessor.pl -O gas-preprocessor.pl
chmod u+x gas-preprocessor.pl
export PATH=$PATH:`pwd`

./configure \
  --prefix=install \
  --disable-doc \
  --disable-programs \
  --disable-autodetect \
  --disable-iconv \
  --disable-avdevice \
  --disable-swresample \
  --disable-postproc \
  --disable-avfilter \
  --disable-network \
  --disable-pixelutils \
  --disable-pthreads \
  --disable-w32threads \
  --disable-everything \
  --enable-decoder=bink \
  --enable-demuxer=bink \
  --enable-decoder=binkaudio_dct \
  --enable-protocol=file \
  --enable-mmx \
  --enable-small \
  "${PLATFORM_FLAGS[@]}"

make
make install

if [ "$PLATFORM" == "win" ]; then
  # Rename .a files to .lib for Windows build
  pushd install/lib
  for f in *.a; do
    mv -- "$f" "${f%.a}.lib"
  done
  popd
fi
