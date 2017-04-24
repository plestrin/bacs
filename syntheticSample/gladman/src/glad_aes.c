#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "aes.h"
#include "aesopt.h"
#include "printBuffer.h"

#ifdef _WIN32
#include "windowsComp.h"
#endif

/* Those test vectors are taken from: http://csrc.nist.gov/publications/fips/fips197/fips-197.pdf */

int main(int argc, char** argv){
	unsigned char plaintext[16] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};

	unsigned char key_128[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
	unsigned char key_192[24] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17};
	unsigned char key_256[32] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};

	aes_encrypt_ctx enc_ctx_128;
	aes_encrypt_ctx enc_ctx_192;
	aes_encrypt_ctx enc_ctx_256;

	aes_decrypt_ctx dec_ctx_128;
	aes_decrypt_ctx dec_ctx_192;
	aes_decrypt_ctx dec_ctx_256;

	unsigned char ciphertext[16];
	unsigned char deciphertext[16];

	#include "print_config.c"

	if (aes_init() != EXIT_SUCCESS){
		printf("ERROR: in %s, aes_init failed\n", __func__);
		return EXIT_FAILURE;
	}

	printf("Plaintext:      ");
	fprintBuffer_raw(stdout, (char*)plaintext, 16);

	/* 128 bits AES */
	printf("\nKey 128:        ");
	fprintBuffer_raw(stdout, (char*)key_128, 16);

	if (aes_encrypt_key128(key_128, &enc_ctx_128) != EXIT_SUCCESS){
		printf("\nERROR: in %s, aes_encrypt_key128 failed\n", __func__);
		return EXIT_FAILURE;
	}

	if (aes_ecb_encrypt(plaintext, ciphertext, 16, &enc_ctx_128) != EXIT_SUCCESS){
		printf("\nERROR: in %s, aes_ecb_encrypt failed\n", __func__);
		return EXIT_FAILURE;
	}

	printf("\nCiphertext 128: ");
	fprintBuffer_raw(stdout, (char*)ciphertext, 16);

	if (aes_decrypt_key128(key_128, &dec_ctx_128) != EXIT_SUCCESS){
		printf("\nERROR: in %s, aes_decrypt_key128 failed\n", __func__);
		return EXIT_FAILURE;
	}

	if (aes_ecb_decrypt(ciphertext, deciphertext, 16, &dec_ctx_128) != EXIT_SUCCESS){
		printf("\nERROR: in %s, aes_ecb_decrypt failed\n", __func__);
		return EXIT_FAILURE;
	}

	if (memcmp(deciphertext, plaintext, 16) == 0){
		printf("\nRecovery 128:   OK");
	}
	else{
		printf("\nRecovery 128:   FAIL");
	}

	/* 192 bits AES */
	printf("\nKey 192:        ");
	fprintBuffer_raw(stdout, (char*)key_192, 24);

	if (aes_encrypt_key192(key_192, &enc_ctx_192) != EXIT_SUCCESS){
		printf("\nERROR: in %s, aes_encrypt_key192 failed\n", __func__);
		return EXIT_FAILURE;
	}

	if (aes_ecb_encrypt(plaintext, ciphertext, 16, &enc_ctx_192) != EXIT_SUCCESS){
		printf("\nERROR: in %s, aes_ecb_encrypt failed\n", __func__);
		return EXIT_FAILURE;
	}

	printf("\nCiphertext 192: ");
	fprintBuffer_raw(stdout, (char*)ciphertext, 16);

 	if (aes_decrypt_key192(key_192, &dec_ctx_192) != EXIT_SUCCESS){
		printf("\nERROR: in %s, aes_decrypt_key192 failed\n", __func__);
		return EXIT_FAILURE;
	}

	if (aes_ecb_decrypt(ciphertext, deciphertext, 16, &dec_ctx_192) != EXIT_SUCCESS){
		printf("\nERROR: in %s, aes_ecb_decrypt failed\n", __func__);
		return EXIT_FAILURE;
	}

	if (memcmp(deciphertext, plaintext, 16) == 0){
		printf("\nRecovery 192:   OK");
	}
	else{
		printf("\nRecovery 192:   FAIL");
	}

	/* 256 bits AES */
	printf("\nKey 256:        ");
	fprintBuffer_raw(stdout, (char*)key_256, 32);

	if (aes_encrypt_key256(key_256, &enc_ctx_256) != EXIT_SUCCESS){
		printf("\nERROR: in %s, aes_encrypt_key256 failed\n", __func__);
		return EXIT_FAILURE;
	}

	if (aes_ecb_encrypt(plaintext, ciphertext, 16, &enc_ctx_256) != EXIT_SUCCESS){
		printf("\nERROR: in %s, aes_ecb_encrypt failed\n", __func__);
		return EXIT_FAILURE;
	}

	printf("\nCiphertext 256: ");
	fprintBuffer_raw(stdout, (char*)ciphertext, 16);

	if (aes_decrypt_key256(key_256, &dec_ctx_256) != EXIT_SUCCESS){
		printf("\nERROR: in %s, aes_decrypt_key256 failed\n", __func__);
		return EXIT_FAILURE;
	}

	if (aes_ecb_decrypt(ciphertext, deciphertext, 16, &dec_ctx_256) != EXIT_SUCCESS){
		printf("\nERROR: in %s, aes_ecb_decrypt failed\n", __func__);
		return EXIT_FAILURE;
	}

	if (memcmp(deciphertext, plaintext, 16) == 0){
		printf("\nRecovery 256:   OK\n");
	}
	else{
		printf("\nRecovery 256:   FAIL\n");
	}

	return EXIT_SUCCESS;
}
