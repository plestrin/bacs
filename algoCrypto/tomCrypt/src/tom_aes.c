#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <tomcrypt.h>

#include "printBuffer.h"

int main(){
	unsigned char 	key128[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
	unsigned char 	key192[24] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17};
	unsigned char 	key256[32] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};

	unsigned char 	ct[16];
	unsigned char 	vt[16];
	unsigned char 	pt[16] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
	symmetric_key 	skey128;
	symmetric_key 	skey192;
	symmetric_key 	skey256;


	/* AES 128 */
	if (aes_setup(key128, 16, 10, &skey128) != CRYPT_OK){
		printf("ERROR: in %s, unable to setup AES 128 key\n", __func__);
		return 0;
	}

	if (aes_ecb_encrypt(pt, ct, &skey128) != CRYPT_OK){
		printf("ERROR: in %s, unable to encrypt AES 128\n", __func__);
	}

	printf("Plaintext:      ");
	fprintBuffer_raw(stdout, (char*)pt, 16);
	printf("\nKey 128:        ");
	fprintBuffer_raw(stdout, (char*)key128, 16);
	printf("\nCiphertext 128: ");
	fprintBuffer_raw(stdout, (char*)ct, 16);

	if (aes_ecb_decrypt(ct, vt, &skey128) != CRYPT_OK){
		printf("ERROR: in %s, unable to decrypt AES 128\n", __func__);
	}

	if (!memcmp(vt, pt, 16)){
		printf("\nRecovery 128:   OK\n");
	}
	else{
		printf("\nRecovery 128:   FAIL\n");
	}

	aes_done(&skey128);


	/* AES 192 */
	if (aes_setup(key192, 24, 12, &skey192) != CRYPT_OK){
		printf("ERROR: in %s, unable to setup AES 192 key\n", __func__);
		return 0;
	}

	if (aes_ecb_encrypt(pt, ct, &skey192) != CRYPT_OK){
		printf("ERROR: in %s, unable to encrypt AES 192\n", __func__);
	}

	printf("Key 192:        ");
	fprintBuffer_raw(stdout, (char*)key192, 24);
	printf("\nCiphertext 192: ");
	fprintBuffer_raw(stdout, (char*)ct, 16);

	if (aes_ecb_decrypt(ct, vt, &skey192) != CRYPT_OK){
		printf("ERROR: in %s, unable to decrypt AES 192\n", __func__);
	}

	if (!memcmp(vt, pt, 16)){
		printf("\nRecovery 192:   OK\n");
	}
	else{
		printf("\nRecovery 192:   FAIL\n");
	}

	aes_done(&skey192);


	/* AES 256 */
	if (aes_setup(key256, 32, 14, &skey256) != CRYPT_OK){
		printf("ERROR: in %s, unable to setup AES 256 key\n", __func__);
		return 0;
	}

	if (aes_ecb_encrypt(pt, ct, &skey256) != CRYPT_OK){
		printf("ERROR: in %s, unable to encrypt AES 256\n", __func__);
	}

	printf("Key 256:        ");
	fprintBuffer_raw(stdout, (char*)key256, 32);
	printf("\nCiphertext 256: ");
	fprintBuffer_raw(stdout, (char*)ct, 16);

	if (aes_ecb_decrypt(ct, vt, &skey256) != CRYPT_OK){
		printf("ERROR: in %s, unable to decrypt AES 256\n", __func__);
	}

	if (!memcmp(vt, pt, 16)){
		printf("\nRecovery 256:   OK\n");
	}
	else{
		printf("\nRecovery 256:   FAIL\n");
	}

	aes_done(&skey256);

	return 0;
}
