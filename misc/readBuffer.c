#include <stdlib.h>
#include <stdio.h>

#include "readBuffer.h"
#include "base.h"

char* readBuffer_raw(const char* txt, size_t txt_length, char* buffer, size_t* buffer_length){
	char* 	result;
	size_t	i;
	size_t 	j;
	size_t 	buffer_length_;

	if (buffer_length != NULL && *buffer_length > 0){
		buffer_length_ = *buffer_length;
	}
	else{
		buffer_length_ = READBUFFER_RAW_GET_LENGTH(txt_length);
	}

	result = buffer;
	if (result == NULL){
		result = (char*)calloc(buffer_length_, 1);
	}
	
	if (result == NULL){
		log_err("unable to allocate memory");
		return NULL;
	}

	for (i = 0, j = 0; i < txt_length; i++){
		if (j >> 2 == buffer_length_){
			log_err_m("reach end of output buffer, but %u bytes are left to be parsed", txt_length - i);
			break;
		}

		if (txt[i] == '\0'){
			break;
		}

		switch(txt[i]){
			case '0' : {result[j >> 1] |= 0x00 << ((~j & 0x01) << 2); j ++; break;}
			case '1' : {result[j >> 1] |= 0x01 << ((~j & 0x01) << 2); j ++; break;}
			case '2' : {result[j >> 1] |= 0x02 << ((~j & 0x01) << 2); j ++; break;}
			case '3' : {result[j >> 1] |= 0x03 << ((~j & 0x01) << 2); j ++; break;}
			case '4' : {result[j >> 1] |= 0x04 << ((~j & 0x01) << 2); j ++; break;}
			case '5' : {result[j >> 1] |= 0x05 << ((~j & 0x01) << 2); j ++; break;}
			case '6' : {result[j >> 1] |= 0x06 << ((~j & 0x01) << 2); j ++; break;}
			case '7' : {result[j >> 1] |= 0x07 << ((~j & 0x01) << 2); j ++; break;}
			case '8' : {result[j >> 1] |= 0x08 << ((~j & 0x01) << 2); j ++; break;}
			case '9' : {result[j >> 1] |= 0x09 << ((~j & 0x01) << 2); j ++; break;}
			case 'A' : {result[j >> 1] |= 0x0a << ((~j & 0x01) << 2); j ++; break;}
			case 'B' : {result[j >> 1] |= 0x0b << ((~j & 0x01) << 2); j ++; break;}
			case 'C' : {result[j >> 1] |= 0x0c << ((~j & 0x01) << 2); j ++; break;}
			case 'D' : {result[j >> 1] |= 0x0d << ((~j & 0x01) << 2); j ++; break;}
			case 'E' : {result[j >> 1] |= 0x0e << ((~j & 0x01) << 2); j ++; break;}
			case 'F' : {result[j >> 1] |= 0x0f << ((~j & 0x01) << 2); j ++; break;}
			case 'a' : {result[j >> 1] |= 0x0a << ((~j & 0x01) << 2); j ++; break;}
			case 'b' : {result[j >> 1] |= 0x0b << ((~j & 0x01) << 2); j ++; break;}
			case 'c' : {result[j >> 1] |= 0x0c << ((~j & 0x01) << 2); j ++; break;}
			case 'd' : {result[j >> 1] |= 0x0d << ((~j & 0x01) << 2); j ++; break;}
			case 'e' : {result[j >> 1] |= 0x0e << ((~j & 0x01) << 2); j ++; break;}
			case 'f' : {result[j >> 1] |= 0x0f << ((~j & 0x01) << 2); j ++; break;}
		}
	}

	if (buffer_length != NULL){
		*buffer_length = j >> 1;
	}

	return result;
}

void readBuffer_reverse_endianness(char* buffer, size_t buffer_length){
	uint64_t i;

	if (buffer_length % 4){
		log_err("buffer size (in byte) must be a multiple of 4");
		return;
	}

	for (i = 0; i < buffer_length; i += 4){
		*(uint32_t*)(buffer + i) = __builtin_bswap32(*(uint32_t*)(buffer + i));
	}
}