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

int main(void){
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

	printf("| NAME           | VALUE         | DESC                                 \n");
	printf("|----------------|---------------|--------------------------------------\n");

	#if ENC_UNROLL == FULL
	printf("| ENC_UNROLL     | FULL          |                                      \n");
	#elif ENC_UNROLL == PARTIAL
	printf("| ENC_UNROLL     | PARTIAL       |                                      \n");
	#else
	printf("| ENC_UNROLL     | NONE          |                                      \n");
	#endif

	#if DEC_UNROLL == FULL
	printf("| DEC_UNROLL     | FULL          |                                      \n");
	#elif DEC_UNROLL  == PARTIAL
	printf("| DEC_UNROLL     | PARTIAL       |                                      \n");
	#else
	printf("| DEC_UNROLL     | NONE          |                                      \n");
	#endif

	#ifdef ENC_KS_UNROLL
	printf("| ENC_KS_UNROLL  | --def--       |                                      \n");
	#else
	printf("| ENC_KS_UNROLL  | --undef--     |                                      \n");
	#endif

	#ifdef DEC_KS_UNROLL
	printf("| DEC_KS_UNROLL  | --def--       |                                      \n");
	#else
	printf("| DEC_KS_UNROLL  | --undef--     |                                      \n");
	#endif

	#ifdef FIXED_TABLES
	printf("| FIXED_TABLES   | --def--       | The tables used by the code are compiled statically into the binary file\n");
	#else
	printf("| FIXED_TABLES   | --undef--     | The subroutine aes_init() must be called to compute them before the code is first used\n");
	#endif

	#if ENC_ROUND == FOUR_TABLES
	printf("| ENC_ROUND      | FOUR_TABLES   | Set tables for the normal encryption round\n");
	#elif ENC_ROUND == ONE_TABLE
	printf("| ENC_ROUND      | ONE_TABLE     | Set tables for the normal encryption round\n");
	#else
	printf("| ENC_ROUND      | NO_TABLES     | Set tables for the normal encryption round\n");
	#endif

	#if LAST_ENC_ROUND == FOUR_TABLES
	printf("| LAST_ENC_ROUND | FOUR_TABLES   | Set tables for the last encryption round\n");
	#elif LAST_ENC_ROUND == ONE_TABLE
	printf("| LAST_ENC_ROUND | ONE_TABLE     | Set tables for the last encryption round\n");
	#else
	printf("| LAST_ENC_ROUND | NO_TABLES     | Set tables for the last encryption round\n");
	#endif

	#if DEC_ROUND == FOUR_TABLES
	printf("| DEC_ROUND      | FOUR_TABLES   | Set tables for the normal decryption round\n");
	#elif DEC_ROUND == ONE_TABLE
	printf("| DEC_ROUND      | ONE_TABLE     | Set tables for the normal decryption round\n");
	#else
	printf("| DEC_ROUND      | NO_TABLES     | Set tables for the normal decryption round\n");
	#endif

	#if LAST_DEC_ROUND == FOUR_TABLES
	printf("| LAST_DEC_ROUND | FOUR_TABLES   | Set tables for the last decryption round\n");
	#elif LAST_DEC_ROUND == ONE_TABLE
	printf("| LAST_DEC_ROUND | ONE_TABLE     | Set tables for the last decryption round\n");
	#else
	printf("| LAST_DEC_ROUND | NO_TABLES     | Set tables for the last decryption round\n");
	#endif

	#if KEY_SCHED == FOUR_TABLES
	printf("| KEY_SCHED      | FOUR_TABLES   | Set tables for the key schedule\n");
	#elif KEY_SCHED == ONE_TABLE
	printf("| KEY_SCHED      | ONE_TABLE     | Set tables for the key schedule\n");
	#else
	printf("| KEY_SCHED      | NO_TABLES     | Set tables for the key schedule\n");
	#endif

	if (aes_init() != EXIT_SUCCESS){
		printf("ERROR: in %s, aes_init failed\n", __func__);
		return EXIT_FAILURE;
	}
	
	printf("\nPlaintext:      ");
	printBuffer_raw(stdout, (char*)plaintext, 16);

	/* 128 bits AES */
	printf("\nKey 128:        ");
	printBuffer_raw(stdout, (char*)key_128, 16);

	if (aes_encrypt_key128(key_128, &enc_ctx_128) != EXIT_SUCCESS){
		printf("ERROR: in %s, aes_encrypt_key128 failed\n", __func__);
		return EXIT_FAILURE;
	}

	if (aes_ecb_encrypt(plaintext, ciphertext, 16, &enc_ctx_128) != EXIT_SUCCESS){
		printf("ERROR: in %s, aes_ecb_encrypt failed\n", __func__);
		return EXIT_FAILURE;
	}

	printf("\nCiphertext 128: ");
	printBuffer_raw(stdout, (char*)ciphertext, 16);

	if (aes_decrypt_key128(key_128, &dec_ctx_128) != EXIT_SUCCESS){
		printf("ERROR: in %s, aes_decrypt_key128 failed\n", __func__);
		return EXIT_FAILURE;
	}

	if (aes_ecb_decrypt(ciphertext, deciphertext, 16, &dec_ctx_128) != EXIT_SUCCESS){
		printf("ERROR: in %s, aes_ecb_decrypt failed\n", __func__);
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
	printBuffer_raw(stdout, (char*)key_192, 24);

	if (aes_encrypt_key192(key_192, &enc_ctx_192) != EXIT_SUCCESS){
		printf("ERROR: in %s, aes_encrypt_key192 failed\n", __func__);
		return EXIT_FAILURE;
	}

	if (aes_ecb_encrypt(plaintext, ciphertext, 16, &enc_ctx_192) != EXIT_SUCCESS){
		printf("ERROR: in %s, aes_ecb_encrypt failed\n", __func__);
		return EXIT_FAILURE;
	}

	printf("\nCiphertext 192: ");
	printBuffer_raw(stdout, (char*)ciphertext, 16);

 	if (aes_decrypt_key192(key_192, &dec_ctx_192) != EXIT_SUCCESS){
		printf("ERROR: in %s, aes_decrypt_key192 failed\n", __func__);
		return EXIT_FAILURE;
	}

	if (aes_ecb_decrypt(ciphertext, deciphertext, 16, &dec_ctx_192) != EXIT_SUCCESS){
		printf("ERROR: in %s, aes_ecb_decrypt failed\n", __func__);
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
	printBuffer_raw(stdout, (char*)key_256, 32);

	if (aes_encrypt_key256(key_256, &enc_ctx_256) != EXIT_SUCCESS){
		printf("ERROR: in %s, aes_encrypt_key256 failed\n", __func__);
		return EXIT_FAILURE;
	}

	if (aes_ecb_encrypt(plaintext, ciphertext, 16, &enc_ctx_256) != EXIT_SUCCESS){
		printf("ERROR: in %s, aes_ecb_encrypt failed\n", __func__);
		return EXIT_FAILURE;
	}

	printf("\nCiphertext 256: ");
	printBuffer_raw(stdout, (char*)ciphertext, 16);

	if (aes_decrypt_key256(key_256, &dec_ctx_256) != EXIT_SUCCESS){
		printf("ERROR: in %s, aes_decrypt_key256 failed\n", __func__);
		return EXIT_FAILURE;
	}

	if (aes_ecb_decrypt(ciphertext, deciphertext, 16, &dec_ctx_256) != EXIT_SUCCESS){
		printf("ERROR: in %s, aes_ecb_decrypt failed\n", __func__);
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