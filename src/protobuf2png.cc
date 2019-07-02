#include <stdio.h>

#include "png_proto_util.h"

int main (int argc, const char** argv) {
	if (argc == 3) {
		protobuf2png(argv[1], argv[2]);
	} else { //  Wrong number of arguments 
		fprintf(stderr, "usage: protobuf2png input-file output-file\n");
	}
	
	return 0;
} 

