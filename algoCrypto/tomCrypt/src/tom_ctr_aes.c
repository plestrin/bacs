#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <tomcrypt.h>

#include "printBuffer.h"

int main(){
	char 			plaintext[] = "Hi I am an AES CTR test vector distributed on 4 128-bit blocks!";
	unsigned char 	key[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
	unsigned char 	iv[16] = {0x01, 0xff, 0x83, 0xf2, 0xf9, 0x98, 0xba, 0xa4, 0xda, 0xdc, 0xaa, 0xcc, 0x8e, 0x17, 0xa4, 0x1b};
	symmetric_CTR 	ctr;
	unsigned char 	ciphertext[sizeof(plaintext)];
	unsigned char 	deciphertext[sizeof(plaintext)];
	int 			err;

	if (register_cipher(&aes_desc) == -1) {
		printf("Error: in %s, unable to register cipher\n", __func__);
		return 0;
	}

	printf("Plaintext:      \"%s\"\n", plaintext);
	printf("IV:             ");
	printBuffer_raw(stdout, (char*)iv, sizeof(iv));
	printf("\nKey 128:        ");
	printBuffer_raw(stdout, (char*)key, sizeof(key));

	/* ENCRYPT */
	if ((err = ctr_start(find_cipher("aes"), iv, key, sizeof(key), 0, CTR_COUNTER_LITTLE_ENDIAN, &ctr)) != CRYPT_OK){
		printf("ERROR: in %s, %s\n", __func__, error_to_string(err));
		return 0;
	}

	if ((err = ctr_encrypt((unsigned char*)plaintext, ciphertext, sizeof(plaintext), &ctr)) != CRYPT_OK){
		printf("ERROR: in %s, %s\n", __func__, error_to_string(err));
		return 0;
	}

	if ((err = ctr_done(&ctr)) != CRYPT_OK){
		printf("ERROR: in %s, %s\n", __func__, error_to_string(err));
		return 0;
	}

	/* DECRYPT */
	if ((err = ctr_start(find_cipher("aes"), iv, key, sizeof(key), 0, CTR_COUNTER_LITTLE_ENDIAN, &ctr)) != CRYPT_OK){
		printf("ERROR: in %s, %s\n", __func__, error_to_string(err));
		return 0;
	}

	if ((err = ctr_decrypt(ciphertext, deciphertext, sizeof(plaintext), &ctr)) != CRYPT_OK){
		printf("ERROR: in %s, %s\n", __func__, error_to_string(err));
		return 0;
	}

	if ((err = ctr_done(&ctr)) != CRYPT_OK){
		printf("ERROR: in %s, %s\n", __func__, error_to_string(err));
		return 0;
	}

	printf("\nCiphertext CTR: ");
	printBuffer_raw(stdout, (char*)ciphertext, sizeof(plaintext));

	if (memcmp(deciphertext, plaintext, sizeof(plaintext)) == 0){
		printf("\nRecovery:       OK\n");
	}
	else{
		printf("\nRecovery:       FAIL\n");
	}
	return 0;
}
