#include <stdlib.h>
#include <stdio.h>

#include "readBuffer.h"

char* readBuffer_raw(char* txt, uint64_t txt_length){
	char* 		result;
	uint64_t	i;

	result = (char*)calloc(READBUFFER_RAW_GET_LENGTH(txt_length), 1);
	if (result == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
	}
	else{
		for (i = 0; i < txt_length; i++){
			switch(txt[i]){
			case '0' : {result[i >> 1] |= 0x00 << ((~i & 0x01) << 2); break;}
			case '1' : {result[i >> 1] |= 0x01 << ((~i & 0x01) << 2); break;}
			case '2' : {result[i >> 1] |= 0x02 << ((~i & 0x01) << 2); break;}
			case '3' : {result[i >> 1] |= 0x03 << ((~i & 0x01) << 2); break;}
			case '4' : {result[i >> 1] |= 0x04 << ((~i & 0x01) << 2); break;}
			case '5' : {result[i >> 1] |= 0x05 << ((~i & 0x01) << 2); break;}
			case '6' : {result[i >> 1] |= 0x06 << ((~i & 0x01) << 2); break;}
			case '7' : {result[i >> 1] |= 0x07 << ((~i & 0x01) << 2); break;}
			case '8' : {result[i >> 1] |= 0x08 << ((~i & 0x01) << 2); break;}
			case '9' : {result[i >> 1] |= 0x09 << ((~i & 0x01) << 2); break;}
			case 'A' : {result[i >> 1] |= 0x0a << ((~i & 0x01) << 2); break;}
			case 'B' : {result[i >> 1] |= 0x0b << ((~i & 0x01) << 2); break;}
			case 'C' : {result[i >> 1] |= 0x0c << ((~i & 0x01) << 2); break;}
			case 'D' : {result[i >> 1] |= 0x0d << ((~i & 0x01) << 2); break;}
			case 'E' : {result[i >> 1] |= 0x0e << ((~i & 0x01) << 2); break;}
			case 'F' : {result[i >> 1] |= 0x0f << ((~i & 0x01) << 2); break;}
			case 'a' : {result[i >> 1] |= 0x0a << ((~i & 0x01) << 2); break;}
			case 'b' : {result[i >> 1] |= 0x0b << ((~i & 0x01) << 2); break;}
			case 'c' : {result[i >> 1] |= 0x0c << ((~i & 0x01) << 2); break;}
			case 'd' : {result[i >> 1] |= 0x0d << ((~i & 0x01) << 2); break;}
			case 'e' : {result[i >> 1] |= 0x0e << ((~i & 0x01) << 2); break;}
			case 'f' : {result[i >> 1] |= 0x0f << ((~i & 0x01) << 2); break;}
			}
		}
	}

	return result;
}