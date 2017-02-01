#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <openssl/aes.h>

#include "printBuffer.h"

int main(){
	char 			plaintext[] = "Hi I am an AES CFB test vector distributed on 4 128-bit blocks!";
	unsigned char 	key[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
	unsigned char 	iv[AES_BLOCK_SIZE] = {0x01, 0xff, 0x83, 0xf2, 0xf9, 0x98, 0xba, 0xa4, 0xda, 0xdc, 0xaa, 0xcc, 0x8e, 0x17, 0xa4, 0x1b};
	unsigned char 	iv_tmp[AES_BLOCK_SIZE];
	unsigned char 	ciphertext[sizeof(plaintext)];
	unsigned char 	deciphertext[sizeof(plaintext)];
	AES_KEY 		ekey;
	int 			num = 0;

	printf("Plaintext:      \"%s\"\n", plaintext);
	printf("IV:             ");
	fprintBuffer_raw(stdout, (char*)iv, sizeof(iv));
	printf("\nKey 128:        ");
	fprintBuffer_raw(stdout, (char*)key, sizeof(key));

	if (AES_set_encrypt_key(key, 128, &ekey)){
		printf("ERROR: in %s, unable to setup AES encryption key\n", __func__);
		return EXIT_FAILURE;
	}

	memcpy(iv_tmp, iv, AES_BLOCK_SIZE);
	AES_cfb128_encrypt((unsigned char*)plaintext, ciphertext, sizeof(plaintext), &ekey, iv_tmp, &num, AES_ENCRYPT);

	memcpy(iv_tmp, iv, AES_BLOCK_SIZE);
	AES_cfb128_encrypt(ciphertext, deciphertext, sizeof(ciphertext), &ekey, iv_tmp, &num, AES_DECRYPT);

	printf("\nCiphertext CFB: ");
	fprintBuffer_raw(stdout, (char*)ciphertext, sizeof(plaintext));

	if (memcmp(deciphertext, plaintext, sizeof(plaintext)) == 0){
		printf("\nRecovery:       OK\n");
	}
	else{
		printf("\nRecovery:       FAIL\n");
	}

	return EXIT_SUCCESS;
}
