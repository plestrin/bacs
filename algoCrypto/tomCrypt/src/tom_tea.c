#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <tomcrypt.h>

#include "printBuffer.h"

int main(){
	unsigned char 	pt[8]		= {0x45, 0xb7, 0x28, 0xba, 0xd7, 0x8f, 0x1a, 0x1f};
	unsigned char 	ct[8];
	unsigned char 	vt[8];
	unsigned char 	key[16] 	= {0x12, 0x45, 0xf0, 0x6a, 0x45, 0x89, 0xfe, 0x60, 0x50, 0xAA, 0x78, 0x59, 0xf5, 0x69, 0x41, 0xbb};
	symmetric_key 	skey;

	printf("Plaintext:       ");
	fprintBuffer_raw(stdout, (char*)pt, 8);
	printf("\nKey:             ");
	fprintBuffer_raw(stdout, (char*)key, 16);

	if (xtea_setup(key, 16, 32, &skey) != CRYPT_OK){
		printf("ERROR: unable to setup xtea key\n");
		return 0;
	}

	if (xtea_ecb_encrypt(pt, ct, &skey) != CRYPT_OK){
		printf("ERROR: unable to encrypt xtea\n");
		return 0;
	}

	if (xtea_ecb_decrypt(ct, vt, &skey) != CRYPT_OK){
		printf("ERROR: unable to decrypt xtea\n");
		return 0;
	}

	printf("\nCiphertext XTEA: ");
	fprintBuffer_raw(stdout, (char*)ct, 8);

	if (memcmp(pt, vt, 8) == 0){
		printf("\nRecovery:        OK\n");
	}
	else{
		printf("\nRecovery:        FAIL\n");
	}

	return 0;
}
