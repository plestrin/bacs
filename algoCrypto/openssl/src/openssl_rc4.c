#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <openssl/rc4.h>

#include "printBuffer.h"

int main(){
	char 		plaintext[] = "Hello World!";
	char		key[] = "Key";
	RC4_KEY 	rc4_key_enc;
	RC4_KEY 	rc4_key_dec;
	char*		ciphertext;
	char* 		deciphertext;

	ciphertext = (char*)malloc(strlen(plaintext));
	deciphertext = (char*)malloc(strlen(plaintext));

	if (ciphertext != NULL && deciphertext != NULL){
		RC4_set_key(&rc4_key_enc, strlen(key), (unsigned char*)key);

		printf("Plaintext:  \"%s\"\n", plaintext);
		printf("Key:        \"%s\"\n", key);

		RC4(&rc4_key_enc, strlen(plaintext), (unsigned char*)plaintext, (unsigned char*)ciphertext);

		printf("Ciphertext: ");
		printBuffer_raw(stdout, ciphertext, strlen(plaintext));
		printf("\n");

		RC4_set_key(&rc4_key_dec, strlen(key), (unsigned char*)key);

		RC4(&rc4_key_dec, strlen(plaintext), (unsigned char*)ciphertext, (unsigned char*)deciphertext);

		if (memcmp(plaintext, deciphertext, strlen(plaintext)) == 0){
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