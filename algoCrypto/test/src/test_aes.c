#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "AES.h"
#include "printBuffer.h"

#ifdef _WIN32
#include "windowsComp.h"
#endif

/* Those test vectors are taken from: http://csrc.nist.gov/publications/fips/fips197/fips-197.pdf */

int main(void){
	unsigned char plaintext[AES_BLOCK_NB_BYTE] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};

	unsigned char key_128[AES_128_NB_BYTE_KEY] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
	unsigned char key_192[AES_192_NB_BYTE_KEY] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17};
	unsigned char key_256[AES_256_NB_BYTE_KEY] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};

	unsigned char round_key_128[AES_128_NB_BYTE_ROUND_KEY];
	unsigned char round_key_192[AES_192_NB_BYTE_ROUND_KEY];
	unsigned char round_key_256[AES_256_NB_BYTE_ROUND_KEY];

	unsigned char ciphertext[AES_BLOCK_NB_BYTE];
	unsigned char deciphertext[AES_BLOCK_NB_BYTE];


	printf("Plaintext:      ");
	fprintBuffer_raw(stdout, (char*)plaintext, AES_BLOCK_NB_BYTE);

	/* 128 bits AES */
	printf("\nKey 128:        ");
	fprintBuffer_raw(stdout, (char*)key_128, AES_128_NB_BYTE_KEY);

	aes128_key_expand_encrypt((uint32_t*)key_128, (uint32_t*)round_key_128);
	aes128_encrypt((uint32_t*)plaintext, (uint32_t*)round_key_128, (uint32_t*)ciphertext);

	printf("\nCiphertext 128: ");
	fprintBuffer_raw(stdout, (char*)ciphertext, AES_BLOCK_NB_BYTE);

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
	fprintBuffer_raw(stdout, (char*)key_192, AES_192_NB_BYTE_KEY);

	aes192_key_expand_encrypt((uint32_t*)key_192, (uint32_t*)round_key_192);
	aes192_encrypt((uint32_t*)plaintext, (uint32_t*)round_key_192, (uint32_t*)ciphertext);

	printf("\nCiphertext 192: ");
	fprintBuffer_raw(stdout, (char*)ciphertext, AES_BLOCK_NB_BYTE);

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
	fprintBuffer_raw(stdout, (char*)key_256, AES_256_NB_BYTE_KEY);

	aes256_key_expand_encrypt((uint32_t*)key_256, (uint32_t*)round_key_256);
	aes256_encrypt((uint32_t*)plaintext, (uint32_t*)round_key_256, (uint32_t*)ciphertext);

	printf("\nCiphertext 256: ");
	fprintBuffer_raw(stdout, (char*)ciphertext, AES_BLOCK_NB_BYTE);

	aes256_key_expand_decrypt((uint32_t*)key_256, (uint32_t*)round_key_256);
	aes256_decrypt((uint32_t*)ciphertext, (uint32_t*)round_key_256, (uint32_t*)deciphertext);

	if (memcmp(deciphertext, plaintext, AES_BLOCK_NB_BYTE) == 0){
		printf("\nRecovery 256:   OK\n");
	}
	else{
		printf("\nRecovery 256:   FAIL\n");
	}

	return EXIT_SUCCESS;
}
