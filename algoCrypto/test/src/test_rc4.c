#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "RC4.h"
#include "printBuffer.h"

#ifdef WIN32
#include "windowsComp.h"
#endif

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
		printf("Plaintext:  \"%s\"\n", plaintext);
		printf("Key:        \"%s\"\n", key);

		rc4((uint8_t*)plaintext, plaintext_length, (uint8_t*)key, key_length, (uint8_t*)ciphertext);

		printf("Ciphertext: ");
		printBuffer_raw(stdout, ciphertext, plaintext_length);
		printf("\n");

		rc4((uint8_t*)ciphertext, plaintext_length, (uint8_t*)key, key_length, (uint8_t*)deciphertext);

		if (memcmp(plaintext, deciphertext, plaintext_length) == 0){
			printf("Check:      OK\n");
		}
		else{
			printf("Check:      FAIL\n");
		}

		free(ciphertext);
		free(deciphertext);
	}
	else{
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
	}

	return 0;
}