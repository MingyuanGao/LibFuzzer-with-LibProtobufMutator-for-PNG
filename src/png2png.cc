#include <stdio.h>

#include "libprotobuf-mutator/src/libfuzzer/libfuzzer_macro.h"

#define MAX_CHUNK_LENGTH 1000000

/* Read one character (inchar), return octet (c), break if EOF */
#define GETBREAK {            \
	inchar=getc(fp_in);       \
    c=(inchar & 0xffU);       \
    if (inchar != c) break;   \
}

/* Copy the data and crc bytes of a chunk */
#define copy_a_chunk(b1,b2,b3,b4) {                                      \
	if (buf[4] == b1 && buf[5] == b2 && buf[6] == b3 && buf[7] == b4 ) { \
		for (i=0; i<8; i++)                                              \
			putc(buf[i], fp_out);                                        \
		for (i=8; i< length+12; i++) {                                   \
			GETBREAK;                                                    \
			putc(c, fp_out);                                             \
		}                                                                \
		continue;                                                        \
	}                                                                    \
}

int png2png (const char* in_file, const char* out_file) {
	unsigned char buf[MAX_CHUNK_LENGTH];
	unsigned char c;
    int inchar;
	unsigned int i;

	FILE *fp_in;
	FILE *fp_out;

	if ((fp_in = fopen(in_file, "rb")) == NULL)
    	return -1;
	
	if ((fp_out = fopen(out_file, "wb")) == NULL)
    	return -1;
	
	/* Copy 8-byte signature */
	for (i=0; i<8; i++) {
		GETBREAK;
		putc(c, fp_out);
	}
	/* Copy chunks */
	for (;;) {
		/* Read the length */
   		uint32_t length; /* must be 32 bits! */
   		GETBREAK; buf[0] = c; length  = c; length <<= 8;
   		GETBREAK; buf[1] = c; length += c; length <<= 8;
   		GETBREAK; buf[2] = c; length += c; length <<= 8;
   		GETBREAK; buf[3] = c; length += c;
		/* Read the chunkname */
		GETBREAK; buf[4] = c;
		GETBREAK; buf[5] = c;
		GETBREAK; buf[6] = c;
		GETBREAK; buf[7] = c;

		/* Known chunks */		
		copy_a_chunk('I','H','D','R');
		copy_a_chunk('P','L','T','E');
		copy_a_chunk('I','D','A','T');
		copy_a_chunk('I','E','N','D');
        copy_a_chunk('b','K','G','D'); 
        copy_a_chunk('c','H','R','M'); 
        copy_a_chunk('d','S','I','G'); 
        copy_a_chunk('e','X','I','f'); 
        copy_a_chunk('g','A','M','A'); 
        copy_a_chunk('h','I','S','T'); 
        copy_a_chunk('i','C','C','P');
        copy_a_chunk('i','T','X','t'); 
        copy_a_chunk('p','H','Y','s'); 
        copy_a_chunk('s','B','I','T'); 
        copy_a_chunk('s','P','L','T'); 
        copy_a_chunk('s','R','G','B'); 
        copy_a_chunk('s','T','E','R'); 
        copy_a_chunk('t','E','X','t');
        copy_a_chunk('t','I','M','E'); 
        copy_a_chunk('t','R','N','S'); 
        copy_a_chunk('z','T','X','t'); 
        copy_a_chunk('s','C','A','L'); 
        copy_a_chunk('p','C','A','L'); 
        copy_a_chunk('o','F','F','s');

		/* Unknown chunks */
		// Copy length and chunk name 
        for (i=0; i<8; i++)
        	putc(buf[i], fp_out);
        // Copy data bytes and CRC 
        for (i=8; i< length+12; i++) {
        	GETBREAK;
        	putc(c, fp_out);
		}
		
		if (inchar != c) break; // EOF
	} // end of "for"

	fclose(fp_in);
	fclose(fp_out);
	
	return 0;
}

int main (int argc, const char** argv) {
	if (argc == 3) {
		png2png(argv[1], argv[2]);
	} else { //  Wrong number of arguments 
		fprintf(stderr, "usage: pngtopng input-file output-file\n");
	}
	
	return 0;
}

