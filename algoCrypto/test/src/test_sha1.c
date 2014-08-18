#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "SHA1.h"
#include "printBuffer.h"

#ifdef WIN32
#include "windowsComp.h"
#endif

/* This test vector is taken from http://www.di-mgt.com.au/sha_testvectors.html */

int main(){
	char 		message[] = "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu";
	char* 		padded_message;
	uint32_t 	message_size;
	uint32_t 	hash[SHA1_HASH_NB_WORD];
	uint32_t 	expected_hash[SHA1_HASH_NB_WORD] = {0x46249ba4, 0x5b642ca0, 0x95f919f4, 0x259170b6, 0x59a2043a};

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