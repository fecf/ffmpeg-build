
cd FFmpeg
pacman --sync --noconfirm --needed base-devel
pacman --sync --noconfirm --needed p7zip
pacman --sync --noconfirm --needed zlib

wget http://www.tortall.net/projects/yasm/releases/yasm-1.3.0-win64.exe -O /bin/yasm.exe

./configure \
  --extra-cflags="-MD -GL" \
  --extra-cxxflags="-MD -GL" \
  --extra-ldflags="-nodefaultlib:LIBCMT -LTCG" \
  --toolchain=msvc \
  --target-os=win64 \
  --arch=x86_64 \
  --arch=amd64 \
  --cpu=amd64 \
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
  --enable-x86asm

make
make install
mv install/bin/*.lib install/lib/
