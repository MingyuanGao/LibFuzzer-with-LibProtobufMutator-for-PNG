#include <stdio.h>
#include "libprotobuf-mutator/src/libfuzzer/libfuzzer_macro.h"

/* Read one character (inchar), return octet (c), break if EOF */
#define GETBREAK {            \
    inchar=getc(fp_in);       \
    c=(inchar & 0xffU);       \
    if (inchar != c) break;   \
}

// Print PNG info
#define MAX_CHUNK_LENGTH 1000000
void print_png_info(const char* in_file) {
	unsigned char buf[MAX_CHUNK_LENGTH];
	unsigned char c;
        int inchar;
	unsigned int i;

	FILE *fp_in;
	if ((fp_in = fopen(in_file, "rb")) == NULL) {
		printf("Open file failed!\n");
    	        return;
	}
	
	/* Print the 8-byte signature */
	printf("\nPNG Signature: ");
	for (i=0; i<8; i++) {
		GETBREAK;
		printf("%x ",c);
	}
	printf("\n");
	
	/* Print chunks's info */
	for (;;) {
		/* Read the length */
   		uint32_t length; /* must be 32 bits! */
   		GETBREAK; length  = c; length <<= 8;
   		GETBREAK; length += c; length <<= 8;
   		GETBREAK; length += c; length <<= 8;
   		GETBREAK; length += c;
		printf("\nlength:%d",length);
		
		/* Read the chunkname */
		GETBREAK; buf[4] = c;
		GETBREAK; buf[5] = c;
		GETBREAK; buf[6] = c;
		GETBREAK; buf[7] = c;
		printf("\ntype:%c%c%c%c", buf[4], buf[5], buf[6], buf[7]);
        
		// Print data bytes 
		printf("\ndata:");
        	for (i=8; i< length+8; i++) {
        		GETBREAK;
       			printf("%x ", c); 
		}
      	
		// Print crc bytes 
		printf("\ncrc:");
		for (i=length+8; i< length+12; i++) {
        		GETBREAK;
       			printf("%x ", c); 
		}
		printf("\n");

		if (inchar != c) break; // EOF
	} // end of "for"

	fclose(fp_in);
}

int main (int argc, const char** argv) {
	if (argc == 2) {
		print_png_info(argv[1]);
	} else { //  Wrong number of arguments 
		fprintf(stderr, "usage: print_png_info input-file\n");
	}
	
	return 0;
} 

