#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "serpent.h"
#include "printBuffer.h"

#ifdef WIN32
#include "windowsComp.h"
#endif

#define KEY_SIZE 24

int main(){
	unsigned char plaintext[SERPENT_BLOCK_NB_BYTE] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
	unsigned char key[SERPENT_KEY_MAX_NB_BYTE] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	unsigned char round_key[SERPENT_ROUND_KEY_NB_BYTE];
	unsigned char ciphertext[SERPENT_BLOCK_NB_BYTE];
	unsigned char deciphertext[SERPENT_BLOCK_NB_BYTE];

	printf("Plaintext:  ");
	printBuffer_raw(stdout, (char*)plaintext, SERPENT_BLOCK_NB_BYTE);

	printf("\nKey:        ");
	printBuffer_raw(stdout, (char*)key, KEY_SIZE);

	serpent_key_expand((uint32_t*)key, KEY_SIZE * 8, (uint32_t*)round_key);
	serpent_encrypt((uint32_t*)plaintext, (uint32_t*)round_key, (uint32_t*)ciphertext);

	printf("\nCiphertext: ");
	printBuffer_raw(stdout, (char*)ciphertext, SERPENT_BLOCK_NB_BYTE);

	serpent_decrypt((uint32_t*)ciphertext, (uint32_t*)round_key, (uint32_t*)deciphertext);

	if (memcmp(deciphertext, plaintext, SERPENT_BLOCK_NB_BYTE) == 0){
		printf("\nRecovery:   OK\n");
	}
	else{
		printf("\nRecovery:   FAIL\n");
	}

	return 0;
}