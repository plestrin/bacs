#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef __linux__

#include "../SHA1.h"
#include "../../misc/printBuffer.h"

#endif

#ifdef WIN32

#include "SHA1.h"
#include "../../misc/printBuffer.h"

#ifndef __func__
#define __func__ __FUNCTION__
#endif

#endif

/* This test vector is taken from http://www.di-mgt.com.au/sha_testvectors.html */

int main(){
	char 		message[] = "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu";
	char* 		padded_message;
	uint32_t 	message_size;
	uint32_t 	hash[SHA1_HASH_NB_WORD];
	uint32_t 	expected_hash[SHA1_HASH_NB_WORD] = {0xa49b2446, 0xa02c645b, 0xf419f995, 0xb6709125, 0x3a04a259};

	message_size = strlen(message);
	padded_message = (char*)malloc(SHA1_DATA_SIZE_TO_NB_BLOCK(message_size) * SHA1_BLOCK_NB_BYTE);
	if (padded_message != NULL){
		memcpy(padded_message, message, message_size);

		sha1((uint32_t*)padded_message, message_size, hash);

		printf("Plaintext: \"%s\"\n", message);
		printf("SHA1 hash: ");
		printBuffer_raw(stdout, (char*)hash, SHA1_HASH_NB_BYTE);
		printf("\n");

		if (!memcmp(expected_hash, hash, SHA1_HASH_NB_BYTE)){
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