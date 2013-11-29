#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "../RC4.h"
#include "../../misc/printBuffer.h"

int main(){
	char 	plaintext[] = "Hello World!";
	char	key[] = "Key";
	char*	ciphertext;
	char* 	deciphertext;

	uint64_t plaintext_length = strlen(plaintext);
	uint8_t key_length = strlen(key);

	ciphertext = (char*)malloc(plaintext_length);
	deciphertext = (char*)malloc(plaintext_length);

	if (ciphertext != NULL && deciphertext != NULL){
		printf("Plaintext: \t%s\n", plaintext);
		printf("Key: \t\t%s\n", key);

		rc4((uint8_t*)plaintext, plaintext_length, (uint8_t*)key, key_length, (uint8_t*)ciphertext);

		printf("Ciphertext: \t");
		printBuffer_raw(stdout, ciphertext, plaintext_length);
		printf("\n");

		rc4((uint8_t*)ciphertext, plaintext_length, (uint8_t*)key, key_length, (uint8_t*)deciphertext);

		if (memcmp(plaintext, deciphertext, plaintext_length) == 0){
			printf("Recovery: \tOK\n");
		}
		else{
			printf("Recovery: \tFAIL\n");
		}


		free(ciphertext);
		free(deciphertext);
	}
	else{
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
	}


	return 0;
}