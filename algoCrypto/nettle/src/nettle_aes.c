#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <nettle/aes.h>

#include "printBuffer.h"

int main(){
	uint8_t 		key128[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
	uint8_t 		key192[24] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17};
	uint8_t 		key256[32] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};

	unsigned char 	ct[16];
	unsigned char 	vt[16];
	unsigned char 	pt[16] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
	struct aes_ctx 	ctx;


	/* AES 128 */
	aes_set_encrypt_key(&ctx, sizeof(key128), key128);
	aes_encrypt(&ctx, sizeof(pt), ct, pt);
	aes_set_decrypt_key(&ctx, sizeof(key128), key128);
	aes_decrypt(&ctx, sizeof(ct), vt, ct);

	printf("Plaintext:      ");
	printBuffer_raw(stdout, (char*)pt, 16);
	printf("\nKey 128:        ");
	printBuffer_raw(stdout, (char*)key128, 16);
	printf("\nCiphertext 128: ");
	printBuffer_raw(stdout, (char*)ct, 16);

	if (!memcmp(vt, pt, 16)){
		printf("\nRecovery 128:   OK\n");
	}
	else{
		printf("\nRecovery 128:   FAIL\n");
	}

	/* AES 192 */
	aes_set_encrypt_key(&ctx, sizeof(key192), key192);
	aes_encrypt(&ctx, sizeof(pt), ct, pt);
	aes_set_decrypt_key(&ctx, sizeof(key192), key192);
	aes_decrypt(&ctx, sizeof(ct), vt, ct);

	printf("Key 192:        ");
	printBuffer_raw(stdout, (char*)key192, 24);
	printf("\nCiphertext 192: ");
	printBuffer_raw(stdout, (char*)ct, 16);

	if (!memcmp(vt, pt, 16)){
		printf("\nRecovery 192:   OK\n");
	}
	else{
		printf("\nRecovery 192:   FAIL\n");
	}


	/* AES 256 */
	aes_set_encrypt_key(&ctx, sizeof(key256), key256);
	aes_encrypt(&ctx, sizeof(pt), ct, pt);
	aes_set_decrypt_key(&ctx, sizeof(key256), key256);
	aes_decrypt(&ctx, sizeof(ct), vt, ct);

	printf("Key 256:        ");
	printBuffer_raw(stdout, (char*)key256, 32);
	printf("\nCiphertext 256: ");
	printBuffer_raw(stdout, (char*)ct, 16);

	if (!memcmp(vt, pt, 16)){
		printf("\nRecovery 256:   OK\n");
	}
	else{
		printf("\nRecovery 256:   FAIL\n");
	}

	return EXIT_SUCCESS;
}
