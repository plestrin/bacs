#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <openssl/aes.h>

#include "printBuffer.h"

static void wrapper_aes_ecb_encrypt(const unsigned char *in, unsigned char *out, size_t length, const AES_KEY *key){
	uint32_t i;

	for (i = 0; i < length / AES_BLOCK_SIZE; i++){
		AES_ecb_encrypt(in, out, key, AES_ENCRYPT);
		in += AES_BLOCK_SIZE;
		out += AES_BLOCK_SIZE;
	}
}

static void wrapper_aes_ecb_decrypt(const unsigned char *in, unsigned char *out, size_t length, const AES_KEY *key){
	uint32_t i;

	for (i = 0; i < length / AES_BLOCK_SIZE; i++){
		AES_ecb_encrypt(in, out, key, AES_DECRYPT);
		in += AES_BLOCK_SIZE;
		out += AES_BLOCK_SIZE;
	}
}

int main(){
	char 			plaintext[] = "Hi I am an AES ECB test vector distributed on 4 128-bit blocks!";
	unsigned char 	key[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
	unsigned char 	ciphertext[sizeof(plaintext)];
	unsigned char 	deciphertext[sizeof(plaintext)];
	AES_KEY 		ekey;
	AES_KEY 		dkey;

	printf("Plaintext:      \"%s\"\n", plaintext);
	printf("Key 128:        ");
	fprintBuffer_raw(stdout, (char*)key, sizeof(key));

	/* ENCRYPT */
	if (AES_set_encrypt_key(key, 128, &ekey)){
		printf("ERROR: in %s, unable to setup AES encryption key\n", __func__);
		return EXIT_FAILURE;
	}

	wrapper_aes_ecb_encrypt((unsigned char*)plaintext, ciphertext, sizeof(plaintext), &ekey);

	/* DECRYPT */
	if (AES_set_decrypt_key(key, 128, &dkey)){
		printf("ERROR: in %s, unable to setup AES dencryption key\n", __func__);
		return EXIT_FAILURE;
	}

	wrapper_aes_ecb_decrypt(ciphertext, deciphertext, sizeof(ciphertext), &dkey);

	printf("\nCiphertext ECB: ");
	fprintBuffer_raw(stdout, (char*)ciphertext, sizeof(plaintext));

	if (memcmp(deciphertext, plaintext, sizeof(plaintext)) == 0){
		printf("\nRecovery:       OK\n");
	}
	else{
		printf("\nRecovery:       FAIL\n");
	}

	return EXIT_SUCCESS;
}
