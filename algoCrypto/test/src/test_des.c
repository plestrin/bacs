#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "DES.h"
#include "printBuffer.h"

int main(){
	uint8_t key[DES_KEY_NB_BYTE] = {0x75, 0x29, 0x79, 0x38, 0x75, 0x92, 0xcb, 0x70};
	uint8_t ct[DES_BLOCK_NB_BYTE];
	uint8_t vt[DES_BLOCK_NB_BYTE];
	uint8_t pt[DES_BLOCK_NB_BYTE] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
	uint8_t round_key[DES_ROUND_KEY_NB_BYTE];
	
	des_key_expand(key, round_key);

	des_encrypt((uint32_t*)pt, (uint32_t*)round_key, (uint32_t*)ct);
	des_decrypt((uint32_t*)ct, (uint32_t*)round_key, (uint32_t*)vt);

	printf("Plaintext:  ");
	printBuffer_raw(stdout, (char*)pt, DES_BLOCK_NB_BYTE);
	printf("\nKey:        ");
	printBuffer_raw(stdout, (char*)key, DES_KEY_NB_BYTE);
	printf("\nCiphertext: ");
	printBuffer_raw(stdout, (char*)ct, DES_BLOCK_NB_BYTE);

	if (!memcmp(vt, pt, 8)){
		printf("\nRecovery:   OK\n");
	}
	else{
		printf("\nRecovery:   FAIL\n");
	}

	return 0;
}
