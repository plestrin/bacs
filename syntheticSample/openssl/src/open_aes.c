#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <openssl/aes.h>

#include "printBuffer.h"

int main(){
	unsigned char 	key128[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
	unsigned char 	key192[24] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17};
	unsigned char 	key256[32] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};

	unsigned char 	ct[16];
	unsigned char 	vt[16];
	unsigned char 	pt[16] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};

	AES_KEY 		ekey128;
	AES_KEY 		dkey128;
	AES_KEY 		ekey192;
	AES_KEY 		dkey192;
	AES_KEY 		ekey256;
	AES_KEY 		dkey256;

	/* AES 128 */
	if (AES_set_encrypt_key(key128, 128, &ekey128)){
		printf("ERROR: in %s, unable to setup AES 128 encrypt key\n", __func__);
		return 0;
	}

	AES_encrypt(pt, ct, &ekey128);

	printf("Plaintext:      ");
	fprintBuffer_raw(stdout, (char*)pt, 16);
	printf("\nKey 128:        ");
	fprintBuffer_raw(stdout, (char*)key128, 16);
	printf("\nCiphertext 128: ");
	fprintBuffer_raw(stdout, (char*)ct, 16);

	if (AES_set_decrypt_key(key128, 128, &dkey128)){
		printf("ERROR: in %s, unable to setup AES 128 decrypt key\n", __func__);
		return 0;
	}

	AES_decrypt(ct, vt, &dkey128);

	if (!memcmp(vt, pt, 16)){
		printf("\nRecovery 128:   OK\n");
	}
	else{
		printf("\nRecovery 128:   FAIL\n");
	}

	/* AES 192 */
	if (AES_set_encrypt_key(key192, 192, &ekey192)){
		printf("ERROR: in %s, unable to setup AES 192 encrypt key\n", __func__);
		return 0;
	}

	AES_encrypt(pt, ct, &ekey192);

	printf("Key 192:        ");
	fprintBuffer_raw(stdout, (char*)key192, 24);
	printf("\nCiphertext 192: ");
	fprintBuffer_raw(stdout, (char*)ct, 16);

	if (AES_set_decrypt_key(key192, 192, &dkey192)){
		printf("ERROR: in %s, unable to setup AES 192 decrypt key\n", __func__);
		return 0;
	}

	AES_decrypt(ct, vt, &dkey192);

	if (!memcmp(vt, pt, 16)){
		printf("\nRecovery 192:   OK\n");
	}
	else{
		printf("\nRecovery 192:   FAIL\n");
	}

	/* AES 256 */
	if (AES_set_encrypt_key(key256, 256, &ekey256)){
		printf("ERROR: in %s, unable to setup AES 256 encrypt key\n", __func__);
		return 0;
	}

	AES_encrypt(pt, ct, &ekey256);

	printf("Key 256:        ");
	fprintBuffer_raw(stdout, (char*)key256, 32);
	printf("\nCiphertext 256: ");
	fprintBuffer_raw(stdout, (char*)ct, 16);

	if (AES_set_decrypt_key(key256, 256, &dkey256)){
		printf("ERROR: in %s, unable to setup AES 256 decrypt key\n", __func__);
		return 0;
	}

	AES_decrypt(ct, vt, &dkey256);

	if (!memcmp(vt, pt, 16)){
		printf("\nRecovery 256:   OK\n");
	}
	else{
		printf("\nRecovery 256:   FAIL\n");
	}

	return 0;
}
