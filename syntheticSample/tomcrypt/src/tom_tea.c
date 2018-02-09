#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <tomcrypt.h>

#include "printBuffer.h"

int main(void){
	unsigned char 	pt[8]		= {0x45, 0xb7, 0x28, 0xba, 0xd7, 0x8f, 0x1a, 0x1f};
	unsigned char 	ct[8];
	unsigned char 	vt[8];
	unsigned char 	key[16] 	= {0x12, 0x45, 0xf0, 0x6a, 0x45, 0x89, 0xfe, 0x60, 0x50, 0xAA, 0x78, 0x59, 0xf5, 0x69, 0x41, 0xbb};
	symmetric_key 	skey;

	printf("Plaintext:       ");
	fprintBuffer_raw(stdout, (char*)pt, sizeof pt);
	printf("\nKey:             ");
	fprintBuffer_raw(stdout, (char*)key, sizeof key);

	if (xtea_setup(key, 16, 32, &skey) != CRYPT_OK){
		printf("ERROR: unable to setup xtea key\n");
		return EXIT_FAILURE;
	}

	if (xtea_ecb_encrypt(pt, ct, &skey) != CRYPT_OK){
		printf("ERROR: unable to encrypt xtea\n");
		return EXIT_FAILURE;
	}

	if (xtea_ecb_decrypt(ct, vt, &skey) != CRYPT_OK){
		printf("ERROR: unable to decrypt xtea\n");
		return EXIT_FAILURE;
	}

	printf("\nCiphertext XTEA: ");
	fprintBuffer_raw(stdout, (char*)ct, sizeof ct);

	if (memcmp(pt, vt, 8)){
		printf("\nRecovery:        FAIL\n");
		return EXIT_FAILURE;
	}

	printf("\nRecovery:        OK\n");

	return EXIT_SUCCESS;
}
