VC_BIN_PATH="/c/Program Files (x86)/Microsoft Visual Studio/2019/Enterprise/VC/Tools/MSVC/14.23.28105/bin/Hostx64/x64"
export PATH=$VC_BIN_PATH:"/c/MinGW/msys/1.0/bin/":$PATH
alias cl=$VC_BIN_PATH/cl.exe
alias link=$VC_BIN_PATH/link.exe
cd FFmpeg
ls
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
  --disable-programs \
  --disable-nvenc \
  --enable-w32threads \
  --enable-x86asm \
  --enable-static \
  --prefix=install \
  --toolchain=msvc

make
make install
rename lib "" install/lib/*.a
rename .a .lib install/lib/*.a

