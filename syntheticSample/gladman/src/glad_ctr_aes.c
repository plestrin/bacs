#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "aes.h"
#include "aesopt.h"
#include "printBuffer.h"

#ifdef _WIN32
#include "windowsComp.h"
#endif

static void inc(unsigned char* buffer){
	uint64_t value;

	value = *(uint64_t*)buffer;
	if (value == 0xffffffffffffffff){
		*(uint64_t*)buffer = 0;
		*(uint64_t*)(buffer + 8) = *(uint64_t*)(buffer + 8) + 1;
	}
	else{
		*(uint64_t*)buffer = value + 1;
	}
}

int main(int argc, char** argv){
	char 			plaintext[] = "Hi I am an AES CTR test vector distributed on 4 128-bit blocks!";
	unsigned char 	key[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
	unsigned char 	iv[16] = {0x01, 0xff, 0x83, 0xf2, 0xf9, 0x98, 0xba, 0xa4, 0xda, 0xdc, 0xaa, 0xcc, 0x8e, 0x17, 0xa4, 0x1b};
	unsigned char 	iv_tmp[sizeof(iv)];
	unsigned char 	ciphertext[sizeof(plaintext)];
	unsigned char 	deciphertext[sizeof(plaintext)];

	aes_encrypt_ctx ctx;

	#include "print_config.c"

	if (aes_init() != EXIT_SUCCESS){
		printf("ERROR: in %s, aes_init failed\n", __func__);
		return EXIT_FAILURE;
	}

	printf("Plaintext:      \"%s\"\n", plaintext);
	printf("IV:             ");
	fprintBuffer_raw(stdout, (char*)iv, sizeof(iv));
	printf("\nKey 128:        ");
	fprintBuffer_raw(stdout, (char*)key, sizeof(key));

	if (aes_encrypt_key128(key, &ctx) != EXIT_SUCCESS){
		printf("\nERROR: in %s, aes_encrypt_key128 failed\n", __func__);
		return EXIT_FAILURE;
	}

	memcpy(iv_tmp, iv, sizeof(iv));

	if (aes_ctr_crypt((unsigned char*)plaintext, ciphertext, sizeof(ciphertext), iv_tmp, inc, &ctx) != EXIT_SUCCESS){
		printf("\nERROR: in %s, aes_cbc_encrypt failed\n", __func__);
		return EXIT_FAILURE;
	}

	printf("\nCiphertext CTR: ");
	fprintBuffer_raw(stdout, (char*)ciphertext, sizeof(ciphertext));

	memcpy(iv_tmp, iv, sizeof(iv));

	if (aes_ctr_crypt(ciphertext, deciphertext, sizeof(ciphertext), iv_tmp, inc, &ctx) != EXIT_SUCCESS){
		printf("\nERROR: in %s, aes_cbc_encrypt failed\n", __func__);
		return EXIT_FAILURE;
	}

	if (!memcmp(deciphertext, plaintext, sizeof(plaintext))){
		printf("\nRecovery:       OK\n");
	}
	else{
		printf("\nRecovery:       FAIL\n");
	}

	return EXIT_SUCCESS;
}
