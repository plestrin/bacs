#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <openssl/des.h>

#include "printBuffer.h"

int main(){
	unsigned char 		key[8] = {0x75, 0x29, 0x79, 0x38, 0x75, 0x92, 0xcb, 0x70};
	unsigned char 		ct[8];
	unsigned char 		vt[8];
	unsigned char 		pt[8] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};

	DES_key_schedule 	key_schedule;

	DES_set_key_checked((DES_cblock*)key, &key_schedule);

	DES_ecb_encrypt((DES_cblock*)pt, (DES_cblock*)ct, &key_schedule, DES_ENCRYPT);

	printf("Plaintext:  ");
	printBuffer_raw(stdout, (char*)pt, 8);
	printf("\nKey:        ");
	printBuffer_raw(stdout, (char*)key, 8);
	printf("\nCiphertext: ");
	printBuffer_raw(stdout, (char*)ct, 8);

	DES_ecb_encrypt((DES_cblock*)ct, (DES_cblock*)vt, &key_schedule, DES_DECRYPT);

	if (!memcmp(vt, pt, 8)){
		printf("\nRecovery:   OK\n");
	}
	else{
		printf("\nRecovery:   FAIL\n");
	}

	return 0;
}