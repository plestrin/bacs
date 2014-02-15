#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef __linux__

#include "MD5.h"
#include "printBuffer.h"

#endif

#ifdef WIN32

#include "MD5.h"
#include "../../misc/printBuffer.h"

#ifndef __func__
#define __func__ __FUNCTION__
#endif

#endif

/* This test vector is taken from http://www.ietf.org/rfc/rfc1321.txt */

int main(){
	char 		message[] = "12345678901234567890123456789012345678901234567890123456789012345678901234567890";
	char* 		padded_message;
	uint32_t 	message_size;
	uint32_t 	hash[MD5_HASH_NB_WORD];
	uint32_t 	expected_hash[MD5_HASH_NB_WORD] = {0xa2f4ed57, 0x55c9e32b, 0x2eda49ac, 0x7ab60721};

	message_size = strlen(message);
	padded_message = (char*)malloc(MD5_DATA_SIZE_TO_NB_BLOCK(message_size) * MD5_BLOCK_NB_BYTE);
	if (padded_message != NULL){
		memcpy(padded_message, message, message_size);

		md5((uint32_t*)padded_message, message_size, hash);

		printf("Plaintext: \"%s\"\n", message);
		printf("MD5 hash:  ");
		printBuffer_raw(stdout, (char*)hash, MD5_HASH_NB_BYTE);
		printf("\n");

		if (!memcmp(expected_hash, hash, MD5_HASH_NB_BYTE)){
			printf("Check:     OK\n");
		}
		else{
			printf("Check:     FAIL\n");
		}

		free(padded_message);
	}
	else{
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
	}

	return 0;
}