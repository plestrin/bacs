#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "twofish.h"
#include "printBuffer.h"

#ifdef WIN32
#include "windowsComp.h"
#endif

int main(){
	uint8_t 			key128[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
	uint8_t 			key192[24] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17};
	uint8_t 			key256[32] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};

	uint8_t 			ct[16];
	uint8_t 			vt[16];
	uint8_t 			pt[16] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
	struct twofishKey 	twofish_key128;
	struct twofishKey 	twofish_key192;
	struct twofishKey 	twofish_key256;

	/* TWOFISH 128 */
	twofish128_key_init((uint32_t*)key128, &twofish_key128);
	twofish_encrypt((uint32_t*)pt, &twofish_key128, (uint32_t*)ct);
	twofish_decrypt((uint32_t*)ct, &twofish_key128, (uint32_t*)vt);

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

	/* TWOFISH 192 */
	twofish192_key_init((uint32_t*)key192, &twofish_key192);
	twofish_encrypt((uint32_t*)pt, &twofish_key192, (uint32_t*)ct);
	twofish_decrypt((uint32_t*)ct, &twofish_key192, (uint32_t*)vt);

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

	/* TWOFISH 256 */
	twofish256_key_init((uint32_t*)key256, &twofish_key256);
	twofish_encrypt((uint32_t*)pt, &twofish_key256, (uint32_t*)ct);
	twofish_decrypt((uint32_t*)ct, &twofish_key256, (uint32_t*)vt);

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

	return 0;
}
