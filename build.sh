echo $PATH
cd FFmpeg
ls
ls /c/Program Files (x86)/Microsoft Visual Studio/2019/Enterprise/VC/Tools/MSVC/
pacman --sync --noconfirm --needed base-devel
pacman --sync --noconfirm --needed p7zip

wget http://www.tortall.net/projects/yasm/releases/yasm-1.3.0-win64.exe -O /bin/yasm.exe

./configure \
  --arch=amd64 \
  --cpu=amd64 \
  --disable-gpl \
  --disable-network \
  --disable-debug \
  --disable-doc \
  --disable-devices \
  --disable-protocols \
  --disable-hwaccels \
  --enable-protocol=file \
  --enable-small \
  --enable-w32threads \
  --enable-x86asm \
  --prefix=install \
  --toolchain=msvc

make
make install
rename lib "" install/lib/*.a
rename .a .lib install/lib/*.a

