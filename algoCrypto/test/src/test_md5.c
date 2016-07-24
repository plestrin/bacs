#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "MD5.h"
#include "printBuffer.h"

#ifdef _WIN32
#include "windowsComp.h"
#endif

/* This test vector is taken from http://www.ietf.org/rfc/rfc1321.txt */

int main(void){
	char 		message[] = "12345678901234567890123456789012345678901234567890123456789012345678901234567890";
	char* 		padded_message;
	uint32_t 	message_size;
	uint32_t 	hash[MD5_HASH_NB_DWORD];

	message_size = strlen(message);
	padded_message = (char*)malloc(MD5_DATA_SIZE_TO_NB_BLOCK(message_size) * MD5_BLOCK_NB_BYTE);
	if (padded_message != NULL){
		memcpy(padded_message, message, message_size);

		md5((uint32_t*)padded_message, message_size, hash);

		printf("Plaintext: \"%s\"\n", message);
		printf("MD5 hash:  ");
		printBuffer_raw(stdout, (char*)hash, MD5_HASH_NB_BYTE);
		printf("\n");

		free(padded_message);
	}
	else{
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
	}

	return EXIT_SUCCESS;
}