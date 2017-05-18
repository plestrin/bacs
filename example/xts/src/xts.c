#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <tomcrypt.h>

#include "printBuffer.h"

int main(void){
	char 			plaintext[] = "Hi I am an AES XTS test vector! Bye";
	unsigned char 	key1[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
	unsigned char 	key2[16] = {0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};
	unsigned char 	tweak[16] = {0x01, 0xff, 0x83, 0xf2, 0xf9, 0x98, 0xba, 0xa4, 0xda, 0xdc, 0xaa, 0xcc, 0x8e, 0x17, 0xa4, 0x1b};
	unsigned char 	ciphertext[sizeof plaintext];
	unsigned char 	deciphertext[sizeof plaintext];
	int 			err;
	symmetric_xts 	xts;

	if (register_cipher(&aes_desc) == -1) {
		printf("Error: in %s, unable to register cipher\n", __func__);
		return EXIT_FAILURE;
	}

	printf("Plaintext:      \"%s\"\n", plaintext);
	printf("Tweak:          ");
	fprintBuffer_raw(stdout, (char*)tweak, sizeof tweak);
	printf("\nKey1 128:       ");
	fprintBuffer_raw(stdout, (char*)key1, sizeof key1);
	printf("\nKey2 128:       ");
	fprintBuffer_raw(stdout, (char*)key2, sizeof key2);

	if ((err = xts_start(find_cipher("aes"), key1, key2, sizeof key1, 10, &xts)) != CRYPT_OK){
		printf("ERROR: in %s, %s\n", __func__, error_to_string(err));
		return EXIT_FAILURE;
	}

	if ((err = xts_encrypt((unsigned char*)plaintext, sizeof plaintext, ciphertext, tweak, &xts)) != CRYPT_OK){
		printf("ERROR: in %s, %s\n", __func__, error_to_string(err));
		return EXIT_FAILURE;
	}

	if ((err = xts_decrypt(ciphertext, sizeof ciphertext, deciphertext, tweak, &xts)) != CRYPT_OK){
		printf("ERROR: in %s, %s\n", __func__, error_to_string(err));
		return EXIT_FAILURE;
	}

	printf("\nCiphertext XTS: ");
	fprintBuffer_raw(stdout, (char*)ciphertext, sizeof plaintext );

	if (!memcmp(deciphertext, plaintext, sizeof plaintext)){
		printf("\nRecovery:       OK\n");
	}
	else{
		printf("\nRecovery:       FAIL\n");
	}

	return EXIT_SUCCESS;
}
