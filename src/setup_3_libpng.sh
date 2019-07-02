rm -rf libpng
git clone  https://github.com/glennrp/libpng.git
git checkout v1.6.0
sed -i '1i#include <stdlib.h>' libpng/contrib/oss-fuzz/libpng_read_fuzzer.cc
cd libpng && ./configure
