#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <tomcrypt.h>

#include "printBuffer.h"

int main(){
	unsigned char 	key[8] = {0x75, 0x29, 0x79, 0x38, 0x75, 0x92, 0xcb, 0x70};
	unsigned char 	ct[8];
	unsigned char 	vt[8];
	unsigned char 	pt[8] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
	symmetric_key 	skey;

	if (des_setup(key, 8, 16, &skey) != CRYPT_OK){
		printf("ERROR: in %s, unable to setup DES key\n", __func__);
		return 0;
	}

	if (des_ecb_encrypt(pt, ct, &skey) != CRYPT_OK){
		printf("ERROR: in %s, unable to encrypt DES\n", __func__);
	}

	printf("Plaintext:  ");
	printBuffer_raw(stdout, (char*)pt, 8);
	printf("\nKey:        ");
	printBuffer_raw(stdout, (char*)key, 8);
	printf("\nCiphertext: ");
	printBuffer_raw(stdout, (char*)ct, 8);

	if (des_ecb_decrypt(ct, vt, &skey) != CRYPT_OK){
		printf("ERROR: in %s, unable to decrypt DES\n", __func__);
	}

	if (!memcmp(vt, pt, 8)){
		printf("\nRecovery:   OK\n");
	}
	else{
		printf("\nRecovery:   FAIL\n");
	}

	des_done(&skey);

	return 0;
}
