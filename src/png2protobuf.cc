#include <stdio.h>

#include "png_proto_util.h"

int main (int argc, const char** argv) {
	if (argc == 3) {
		png2protobuf(argv[1], argv[2]);
	} else { //  Wrong number of arguments 
		fprintf(stderr, "usage: png2protobuf input-file output-file\n");
	}
	
	return 0;
} 

