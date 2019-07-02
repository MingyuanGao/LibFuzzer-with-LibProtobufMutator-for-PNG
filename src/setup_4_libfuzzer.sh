echo "deb http://apt.llvm.org/bionic/ llvm-toolchain-bionic main"   | sudo tee -a /etc/apt/sources.list
echo "deb http://apt.llvm.org/bionic/ llvm-toolchain-bionic-7 main" | sudo tee -a /etc/apt/sources.list
echo "deb http://apt.llvm.org/bionic/ llvm-toolchain-bionic-8 main" | sudo tee -a /etc/apt/sources.list
sudo apt install -y wget gnupg
wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | sudo apt-key add -
sudo apt update
sudo apt install -y clang-9
