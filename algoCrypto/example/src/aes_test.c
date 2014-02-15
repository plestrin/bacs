#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef __linux__

#include "AES.h"
#include "printBuffer.h"

#endif

#ifdef WIN32

#include "AES.h"
#include "../../misc/printBuffer.h"

#ifndef __func__
#define __func__ __FUNCTION__
#endif

#endif

/* Those test vectors are taken from: http://csrc.nist.gov/publications/fips/fips197/fips-197.pdf */

int main(){
	unsigned char plaintext[AES_BLOCK_NB_BYTE] = {0x33, 0x22, 0x11, 0x00, 0x77, 0x66, 0x55, 0x44, 0xbb, 0xaa, 0x99, 0x88, 0xff, 0xee, 0xdd, 0xcc};
	
	unsigned char key_128[AES_128_NB_BYTE_KEY] = {0x03, 0x02, 0x01, 0x00, 0x07, 0x06, 0x05, 0x04, 0x0b, 0x0a, 0x09, 0x08, 0x0f, 0x0e, 0x0d, 0x0c};
	unsigned char key_192[AES_192_NB_BYTE_KEY] = {0x03, 0x02, 0x01, 0x00, 0x07, 0x06, 0x05, 0x04, 0x0b, 0x0a, 0x09, 0x08, 0x0f, 0x0e, 0x0d, 0x0c, 0x13, 0x12, 0x11, 0x10, 0x17, 0x16, 0x15, 0x14};
	unsigned char key_256[AES_256_NB_BYTE_KEY] = {0x03, 0x02, 0x01, 0x00, 0x07, 0x06, 0x05, 0x04, 0x0b, 0x0a, 0x09, 0x08, 0x0f, 0x0e, 0x0d, 0x0c, 0x13, 0x12, 0x11, 0x10, 0x17, 0x16, 0x15, 0x14, 0x1b, 0x1a, 0x19, 0x18, 0x1f, 0x1e, 0x1d, 0x1c};

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