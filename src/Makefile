# Makefile for building a fuzzer with mutator being libprotobuf-mutator 

CC  = clang-9 
CXX = clang++-9

INC_DIRS = -I/usr/local/include/libprotobuf-mutator -Ilibpng
LIB_DIRS = -L/usr/local/lib

# Using static libs
LPM_LIBS = /usr/local/lib/libprotobuf-mutator-libfuzzer.a /usr/local/lib/libprotobuf-mutator.a 
PROTOBUF_LIBS = /usr/local/lib/libprotobuf.a 
PNG_LIBS = libpng/.libs/libpng16.a

COMMON_FLAGS = -g
CFLAGS   = $(COMMON_FLAGS) 
CXXFLAGS = $(COMMON_FLAGS)
FUZZER_FLAGS = -fsanitize=fuzzer,address
FUZZER_FLAGS_NO_LINK = -fsanitize=fuzzer-no-link,address
LDFLAGS  = $(COMMON_FLAGS) $(LIB_DIRS) -lz


##############################
# targets
##############################

all: libpng_read_fuzzer_protobuf-mutator png2protobuf protobuf2png print_png_info png2png 

#---------------------------
fuzzer = libpng_read_fuzzer_protobuf-mutator
fuzzer_objs = png.pb.o libpng_read_fuzzer_protobuf.o libpng_reader_fuzzer.o    

$(fuzzer) : build_libpng_with_fuzzer $(fuzzer_objs)
	$(CXX) -o $@ $(fuzzer_objs) $(FUZZER_FLAGS) $(LDFLAGS) $(LPM_LIBS) $(PROTOBUF_LIBS) $(PNG_LIBS) 

png.pb.cc : png.proto
	protoc --cpp_out=$(PWD) $< 

libpng_reader_fuzzer.o: libpng/contrib/oss-fuzz/libpng_read_fuzzer.cc
	$(CXX) $(CXXFLAGS) $(INC_DIRS) -DLLVMFuzzerTestOneInput=FuzzPNG -c -o $@ $< 

#---------------------------
protobuf2png : png.pb.o png_proto_util.o protobuf2png.o  
	$(CXX) -o $@ $^ $(LDFLAGS) $(PROTOBUF_LIBS) 

#---------------------------
png2protobuf : png.pb.o png_proto_util.o png2protobuf.o  
	$(CXX) -o $@ $^ $(LDFLAGS) $(PROTOBUF_LIBS) 

#---------------------------
print_png_info: print_png_info.o
	$(CXX) -o $@ $^

#---------------------------
png2png: png2png.o
	$(CXX) -o $@ $^

#---------------------------
.PHONY: build_libpng_with_fuzzer  
build_libpng_with_fuzzer: 
	cd libpng && make clean && ./configure && make -j40 CC=$(CC) CXX=$(CXX) CFLAGS="$(COMMON_FLAGS) $(FUZZER_FLAGS_NO_LINK)" CXXFLAGS="$(COMMON_FLAGS) $(FUZZER_FLAGS_NO_LINK)"


##############################
# rules
##############################
%.o : %.c
	$(CC) $(CFLAGS) $(INC_DIRS) -c -o $@ $<

%.o : %.cc
	$(CXX) $(CXXFLAGS) $(INC_DIRS) -c -o $@ $<

%.o : %.cpp
	$(CXX) $(CXXFLAGS) $(INC_DIRS) -c -o $@ $<


##############################
# clean targets
##############################
.PHONY: clean 
clean: 
	rm -f $(fuzzer) protobuf2png png2protobuf print_png_info png2png *.o *.pb.cc *.pb.h *.log crash-* 

