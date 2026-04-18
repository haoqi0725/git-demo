cd ~/Test/Game/build
mkdir build
cd build
cmake ..
make
export LD_LIBRARY_PATH=../raylib-5.0_linux_amd64/lib:$LD_LIBRARY_PATH  # 如果使用预编译包
./shadow_echo

cd ~/Test/Game/build
cmake
export LD_LIBRARY_PATH=../raylib-5.0_linux_amd64/lib:$LD_LIBRARY_PATH
./shadow_echo