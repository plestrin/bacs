#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../readBuffer.h"
#include "../printBuffer.h"
#include "../base.h"

#define BUFFER_SIZE 256

int main(){
	char 		input[BUFFER_SIZE];
	char* 		buffer;
	uint32_t 	input_size;
	uint32_t 	buffer_size;

	printf("Enter some hexa string: ");
	fflush(stdout);

	if (fgets(input, BUFFER_SIZE, stdin) == NULL){
		log_err("gets return NULL");
		return 0;
	}
	input_size = strlen(input);
	input_size --;
	input[input_size] = '\0';

	buffer = readBuffer_raw(input, input_size, NULL);
	buffer_size = READBUFFER_RAW_GET_LENGTH(input_size);
	if (buffer == NULL){
		log_err("readBuffer return NULL");
	}
	else{
		printBuffer_raw(stdout, buffer, buffer_size);
		printf("\n");
		free(buffer);
	}


	return 0;
}