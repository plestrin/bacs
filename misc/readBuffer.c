#include <stdlib.h>
#include <stdio.h>

#include "readBuffer.h"

char* readBuffer_raw(const char* txt, size_t txt_length, char* buffer){
	char* 	result;
	size_t	i;

	result = buffer;
	if (result == NULL){
		result = (char*)calloc(READBUFFER_RAW_GET_LENGTH(txt_length), 1);
	}
	
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

void readBuffer_reverse_endianness(char* buffer, size_t buffer_length){
	uint64_t i;

	if (buffer_length % 4){
		printf("ERROR: in %s, buffer size (in byte) must be a multiple of 4\n", __func__);
		return;
	}

	for (i = 0; i < buffer_length; i += 4){
		*(uint32_t*)(buffer + i) = __builtin_bswap32(*(uint32_t*)(buffer + i));
	}
}