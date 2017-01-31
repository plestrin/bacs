#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "TEA.h"
#include "printBuffer.h"

#ifdef _WIN32
#include "windowsComp.h"
#endif

int main(void){
	unsigned char 	plaintext[TEA_BLOCK_NB_BYTE]		= {0x45, 0xb7, 0x28, 0xba, 0xd7, 0x8f, 0x1a, 0x1f};
	char 			ciphertext[TEA_BLOCK_NB_BYTE];
	char  			deciphertext[TEA_BLOCK_NB_BYTE];
	unsigned char 	key[TEA_KEY_NB_BYTE]	 			= {0x12, 0x45, 0xf0, 0x6a, 0x45, 0x89, 0xfe, 0x60, 0x50, 0xAA, 0x78, 0x59, 0xf5, 0x69, 0x41, 0xbb};

	printf("Plaintext:       ");
	fprintBuffer_raw(stdout, (char*)plaintext, TEA_BLOCK_NB_BYTE);
	printf("\nKey:             ");
	fprintBuffer_raw(stdout, (char*)key, TEA_KEY_NB_BYTE);

	tea_encrypt((uint32_t*)plaintext, (uint32_t*)key, (uint32_t*)ciphertext);
	tea_decrypt((uint32_t*)ciphertext, (uint32_t*)key, (uint32_t*)deciphertext);
	
	printf("\nCiphertext TEA:  ");
	fprintBuffer_raw(stdout, ciphertext, TEA_BLOCK_NB_BYTE);

	if (memcmp(plaintext, deciphertext, TEA_BLOCK_NB_BYTE) == 0){
		printf("\nRecovery:        OK\n");
	}
	else{
		printf("\nRecovery:        FAIL\n");
	}

	xtea_encrypt((uint32_t*)plaintext, (uint32_t*)key, (uint32_t*)ciphertext);
	xtea_decrypt((uint32_t*)ciphertext, (uint32_t*)key, (uint32_t*)deciphertext);

	printf("Ciphertext XTEA: ");
	fprintBuffer_raw(stdout, ciphertext, TEA_BLOCK_NB_BYTE);

	if (memcmp(plaintext, deciphertext, TEA_BLOCK_NB_BYTE) == 0){
		printf("\nRecovery:        OK\n");
	}
	else{
		printf("\nRecovery:        FAIL\n");
	}

	return EXIT_SUCCESS;
}
