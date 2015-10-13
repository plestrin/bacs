#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <tomcrypt.h>

#include "printBuffer.h"

/* Test vectors are taken from http://web.cs.ucdavis.edu/~rogaway/ocb/ocb128.txt */

int main(){
	unsigned char 	plaintext[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21};
	unsigned char 	key[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
	unsigned char 	iv[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
	unsigned char 	ciphertext[sizeof(plaintext)];
	unsigned char 	tag[16];
	unsigned long 	tag_length = sizeof(tag);
	int 			err;

	if (register_cipher(&aes_desc) == -1) {
		printf("Error: in %s, unable to register cipher\n", __func__);
		return EXIT_FAILURE;
	}
	
	printf("Plaintext:      ");
	printBuffer_raw(stdout, (char*)plaintext, sizeof(plaintext));
	printf("\nIV:             ");
	printBuffer_raw(stdout, (char*)iv, sizeof(iv));
	printf("\nKey 128:        ");
	printBuffer_raw(stdout, (char*)key, sizeof(key));

	if ((err = ocb_encrypt_authenticate_memory(find_cipher("aes"), key, sizeof(key), iv, plaintext, sizeof(plaintext), ciphertext, tag, &tag_length)) != CRYPT_OK){
		printf("ERROR: in %s, %s\n", __func__, error_to_string(err));
		return EXIT_FAILURE;
	}

	if ((err = ocb_decrypt_authenticate_memory(find_cipher("aes"), key, sizeof(key), iv, plaintext, sizeof(plaintext), ciphertext, tag, &tag_length)) != CRYPT_OK){
		printf("ERROR: in %s, %s\n", __func__, error_to_string(err));
		return EXIT_FAILURE;
	}

	printf("\nCiphertext OCB: ");
	printBuffer_raw(stdout, (char*)ciphertext, sizeof(plaintext));
	printf("\nTag:            ");
	printBuffer_raw(stdout, (char*)tag, tag_length);
	printf("\n");

	return EXIT_SUCCESS;
}
