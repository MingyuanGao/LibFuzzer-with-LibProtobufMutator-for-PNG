sudo apt install -y autoconf automake libtool curl make g++ unzip git
rm -rf protobuf
git clone https://github.com/protocolbuffers/protobuf.git
cd protobuf
git submodule update --init --recursive
./autogen.sh
./configure
make -j40 
make check
sudo make install
sudo ldconfig
cd ..
rm -rf protobuf
