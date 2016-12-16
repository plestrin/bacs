#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <tomcrypt.h>

#include "printBuffer.h"

int main(){
	char 			plaintext[] = "Hello World!";
	char			key[] = "Key";
	char*			ciphertext;
	char*			deciphertext;
	prng_state 		prng;
	unsigned int 	plaintext_length = strlen(plaintext);

	ciphertext = (char*)malloc(plaintext_length);
	deciphertext = (char*)malloc(plaintext_length);

	if (ciphertext != NULL && deciphertext != NULL){
		memcpy(ciphertext, plaintext, plaintext_length);

		printf("Plaintext:  \"%s\"\n", plaintext);
		printf("Key:        \"%s\"\n", key);

		rc4_start(&prng);
		rc4_add_entropy((unsigned char*)key, strlen(key), &prng);
		rc4_ready(&prng);
		rc4_read((unsigned char*)ciphertext, plaintext_length, &prng);
		rc4_done(&prng);

		printf("Ciphertext: ");
		fprintBuffer_raw(stdout, ciphertext, plaintext_length);
		printf("\n");

		memcpy(deciphertext, ciphertext, plaintext_length);

		rc4_start(&prng);
		rc4_add_entropy((unsigned char*)key, strlen(key), &prng);
		rc4_ready(&prng);
		rc4_read((unsigned char*)deciphertext, plaintext_length, &prng);
		rc4_done(&prng);

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
