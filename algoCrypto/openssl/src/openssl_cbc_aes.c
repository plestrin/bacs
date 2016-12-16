#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <openssl/aes.h>

#include "printBuffer.h"

int main(){
	char 			plaintext[] = "Hi I am an AES CBC test vector distributed on 4 128-bit blocks!";
	unsigned char 	key[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
	unsigned char 	iv[16] = {0x01, 0xff, 0x83, 0xf2, 0xf9, 0x98, 0xba, 0xa4, 0xda, 0xdc, 0xaa, 0xcc, 0x8e, 0x17, 0xa4, 0x1b};
	unsigned char 	iv_tmp[sizeof(iv)];
	unsigned char 	ciphertext[sizeof(plaintext)];
	unsigned char 	deciphertext[sizeof(plaintext)];
	AES_KEY 		ekey;
	AES_KEY 		dkey;

	printf("Plaintext:      \"%s\"\n", plaintext);
	printf("IV:             ");
	fprintBuffer_raw(stdout, (char*)iv, sizeof(iv));
	printf("\nKey 128:        ");
	fprintBuffer_raw(stdout, (char*)key, sizeof(key));

	/* ENCRYPT */
	if (AES_set_encrypt_key(key, 128, &ekey)){
		printf("ERROR: in %s, unable to setup AES encryption key\n", __func__);
		return EXIT_FAILURE;
	}

	memcpy(iv_tmp, iv, sizeof(iv));
	AES_cbc_encrypt((unsigned char*)plaintext, ciphertext, sizeof(plaintext), &ekey, iv_tmp, AES_ENCRYPT);

	/* DECRYPT */
	if (AES_set_decrypt_key(key, 128, &dkey)){
		printf("ERROR: in %s, unable to setup AES decryption key\n", __func__);
		return EXIT_FAILURE;
	}

	memcpy(iv_tmp, iv, sizeof(iv));
	AES_cbc_encrypt(ciphertext, deciphertext, sizeof(ciphertext), &dkey, iv_tmp, AES_DECRYPT);

	printf("\nCiphertext CBC: ");
	fprintBuffer_raw(stdout, (char*)ciphertext, sizeof(plaintext));

	if (memcmp(deciphertext, plaintext, sizeof(plaintext)) == 0){
		printf("\nRecovery:       OK\n");
	}
	else{
		printf("\nRecovery:       FAIL\n");
	}

	return EXIT_SUCCESS;
}
