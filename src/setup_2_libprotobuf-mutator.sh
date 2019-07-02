sudo apt install -y cmake zlib1g-dev 
rm -rf libprotobuf-mutator
git clone --depth 1  https://github.com/google/libprotobuf-mutator.git
cd libprotobuf-mutator
mkdir build 
cd build
cmake ..
make -j40
sudo make install
cd ../../
rm -rf libprotobuf-mutator
