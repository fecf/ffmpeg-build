
cd FFmpeg
pacman --sync --noconfirm --needed base-devel
pacman --sync --noconfirm --needed p7zip
pacman --sync --noconfirm --needed zlib

wget http://www.tortall.net/projects/yasm/releases/yasm-1.3.0-win64.exe -O /bin/yasm.exe
wget https://raw.githubusercontent.com/FFmpeg/gas-preprocessor/master/gas-preprocessor.pl -O gas-preprocessor.pl
export PATH=$PATH$:`pwd`

if [ "$ARCH" == "arm" ]; then
  echo "Compiling for ARM"
  PLATFORM_FLAGS="--target-os=win32 --as=armasm --arch=arm --cpu=armv7  --enable-cross-compile"
  EXTRA_C_FLAGS="-D__ARM_PCS_VFP"
elif [ "$ARCH" == "arm64" ]; then
  echo "Compiling for ARM64"
  PLATFORM_FLAGS="--target-os=win64 --as=armasm64 --arch=aarch64 --enable-cross-compile"
  EXTRA_C_FLAGS=
elif [ "$ARCH" == "x86" ]; then
  echo "Compiling for x86"
  PLATFORM_FLAGS="--target-os=win32 --enable-x86asm --arch=i686 --cpu=x86"
  EXTRA_C_FLAGS=
else
  echo "Compiling for x64"
  PLATFORM_FLAGS="--target-os=win64 --enable-x86asm --arch=amd64 --cpu=amd64"
  EXTRA_C_FLAGS=
fi

./configure \
  --extra-cflags="-MD -GL ${EXTRA_C_FLAGS}" \
  --extra-cxxflags="-MD -GL" \
  --extra-ldflags="-nodefaultlib:LIBCMT -LTCG" \
  --toolchain=msvc \
  --prefix=install \
  --disable-doc \
  --disable-programs \
  --disable-autodetect \
  --disable-iconv \
  --disable-avdevice \
  --disable-swresample \
  --disable-swscale \
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
  --enable-small \
  $PLATFORM_FLAGS

make
make install

# Rename .a files to .lib for Windows build
pushd install/lib
for f in *.a; do
  mv -- "$f" "${f%.a}.lib"
done
popd
