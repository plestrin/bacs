#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <tomcrypt.h>


#include "../../misc/printBuffer.h"

int main(){
	char 			plaintext[] = "Hello World!";
	char			key[] = "Key";
	char*			ciphertext;
	prng_state 		prng;
	unsigned int 	plaintext_length = strlen(plaintext);

	ciphertext = (char*)malloc(plaintext_length);

	if (ciphertext != NULL){
		memcpy(ciphertext, plaintext, plaintext_length);

		printf("Plaintext: \t%s\n", plaintext);
		printf("Key: \t\t%s\n", key);

		rc4_start(&prng);
		rc4_add_entropy((unsigned char*)key, strlen(key), &prng);
		rc4_ready(&prng);
		rc4_read((unsigned char*)ciphertext, plaintext_length, &prng);
		rc4_done(&prng);

		printf("Ciphertext: \t");
		printBuffer_raw(stdout, ciphertext, plaintext_length);
		printf("\n");

		free(ciphertext);
	}
	else{
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
	}


	return 0;
}