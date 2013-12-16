#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef __linux__

#include "../AES.h"
#include "../../misc/printBuffer.h"

#endif

#ifdef WIN32

#include "AES.h"
#include "../../misc/printBuffer.h"

#ifndef __func__
#define __func__ __FUNCTION__
#endif

#endif

/*Those test vectors are taken from: http://www.inconteam.com/software-development/41-encryption/55-aes-test-vectors#aes-ecb */

int main(){
	unsigned char plaintext[AES_BLOCK_NB_BYTE] = {0xe2, 0xbe, 0xc1, 0x6b, 0x96, 0x9f, 0x40, 0x2e, 0x11, 0x7e, 0x3d, 0xe9, 0x2a, 0x17, 0x93, 0x73};
	
	unsigned char key_128[AES_128_NB_BYTE_KEY] = {0x16, 0x15, 0x7e, 0x2b, 0xa6, 0xd2, 0xae, 0x28, 0x88, 0x15, 0xf7, 0xab, 0x3c, 0x4f, 0xcf, 0x09};
	unsigned char key_192[AES_192_NB_BYTE_KEY] = {0xf7, 0xb0, 0x73, 0x8e, 0x52, 0x64, 0x0e, 0xda, 0x2b, 0xf3, 0x10, 0xc8, 0xe5, 0x79, 0x90, 0x80, 0xd2, 0xea, 0xf8, 0x62, 0x7b, 0x6b, 0x2c, 0x52};
	unsigned char key_256[AES_256_NB_BYTE_KEY] = {0x10, 0xeb, 0x3d, 0x60, 0xbe, 0x71, 0xca, 0x15, 0xf0, 0xae, 0x73, 0x2b, 0x81, 0x77, 0x7d, 0x85, 0x07, 0x2c, 0x35, 0x1f, 0xd7, 0x08, 0x61, 0x3b, 0xa3, 0x10, 0x98, 0x2d, 0xf4, 0xdf, 0x14, 0x09};

	unsigned char round_key_128[AES_128_NB_BYTE_ROUND_KEY];
	unsigned char round_key_192[AES_192_NB_BYTE_ROUND_KEY];
	unsigned char round_key_256[AES_256_NB_BYTE_ROUND_KEY];

	unsigned char ciphertext[AES_BLOCK_NB_BYTE];
	unsigned char deciphertext[AES_BLOCK_NB_BYTE];

	
	printf("Plaintext:      ");
	printBuffer_raw(stdout, (char*)plaintext, AES_BLOCK_NB_BYTE);

	/* 128 bits AES */
	printf("\nKey 128:        ");
	printBuffer_raw(stdout, (char*)key_128, AES_128_NB_BYTE_KEY);

	aes128_key_expand_encrypt((uint32_t*)key_128, (uint32_t*)round_key_128);
	aes128_encrypt((uint32_t*)plaintext, (uint32_t*)round_key_128, (uint32_t*)ciphertext);

	printf("\nCiphertext 128: ");
	printBuffer_raw(stdout, (char*)ciphertext, AES_BLOCK_NB_BYTE);

 	aes128_key_expand_decrypt((uint32_t*)key_128, (uint32_t*)round_key_128);
	aes128_decrypt((uint32_t*)ciphertext, (uint32_t*)round_key_128, (uint32_t*)deciphertext);

	if (memcmp(deciphertext, plaintext, AES_BLOCK_NB_BYTE) == 0){
		printf("\nRecovery 128:   OK");
	}
	else{
		printf("\nRecovery 128:   FAIL");
	}

	/* 192 bits AES */
	printf("\nKey 192:        ");
	printBuffer_raw(stdout, (char*)key_192, AES_192_NB_BYTE_KEY);

	aes192_key_expand_encrypt((uint32_t*)key_192, (uint32_t*)round_key_192);
	aes192_encrypt((uint32_t*)plaintext, (uint32_t*)round_key_192, (uint32_t*)ciphertext);

	printf("\nCiphertext 192: ");
	printBuffer_raw(stdout, (char*)ciphertext, AES_BLOCK_NB_BYTE);

 	aes192_key_expand_decrypt((uint32_t*)key_192, (uint32_t*)round_key_192);
	aes192_decrypt((uint32_t*)ciphertext, (uint32_t*)round_key_192, (uint32_t*)deciphertext);

	if (memcmp(deciphertext, plaintext, AES_BLOCK_NB_BYTE) == 0){
		printf("\nRecovery 192:   OK");
	}
	else{
		printf("\nRecovery 192:   FAIL");
	}

	/* 256 bits AES */
	printf("\nKey 256:        ");
	printBuffer_raw(stdout, (char*)key_256, AES_256_NB_BYTE_KEY);

	aes256_key_expand_encrypt((uint32_t*)key_256, (uint32_t*)round_key_256);
	aes256_encrypt((uint32_t*)plaintext, (uint32_t*)round_key_256, (uint32_t*)ciphertext);

	printf("\nCiphertext 256: ");
	printBuffer_raw(stdout, (char*)ciphertext, AES_BLOCK_NB_BYTE);

	aes256_key_expand_decrypt((uint32_t*)key_256, (uint32_t*)round_key_256);
	aes256_decrypt((uint32_t*)ciphertext, (uint32_t*)round_key_256, (uint32_t*)deciphertext);

	if (memcmp(deciphertext, plaintext, AES_BLOCK_NB_BYTE) == 0){
		printf("\nRecovery 256:   OK\n");
	}
	else{
		printf("\nRecovery 256:   FAIL\n");
	}

	return 0;
}