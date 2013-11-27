#include <stdlib.h>
#include <stdio.h>

#include "printBuffer.h"

void printBuffer_raw(FILE* file, char* buffer, uint64_t buffer_length){
	uint64_t 	i;
	char 		hexa[16] = {'0', '1', '2', '3', '4' , '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

	for (i = 0; i < buffer_length; i ++){
		fprintf(file, "%c%c", hexa[(buffer[i] >> 4) & 0x0f], hexa[buffer[i] & 0x0f]);
	}
}