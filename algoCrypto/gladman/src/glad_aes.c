#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "aes.h"
#include "aesopt.h"
#include "printBuffer.h"
#include "multiColumn.h"

#ifdef WIN32
#include "windowsComp.h"
#endif

/* Those test vectors are taken from: http://csrc.nist.gov/publications/fips/fips197/fips-197.pdf */

int main(){
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

	struct multiColumnPrinter* 	printer;

	printer = multiColumnPrinter_create(stdout, 3, NULL, NULL, NULL);
	if (printer != NULL){
		multiColumnPrinter_set_column_size(printer, 0, 24);
		multiColumnPrinter_set_column_size(printer, 2, 64);

		multiColumnPrinter_set_title(printer, 0, (char*)"NAME");
		multiColumnPrinter_set_title(printer, 1, (char*)"VALUE");
		multiColumnPrinter_set_title(printer, 2, (char*)"DESC");

		multiColumnPrinter_set_column_type(printer, 2, MULTICOLUMN_TYPE_UNBOUND_STRING);

		multiColumnPrinter_print_header(printer);

		#if ENC_UNROLL == FULL
		multiColumnPrinter_print(printer, "ENC_UNROLL", "FULL", "", NULL);
		#elif ENC_UNROLL == PARTIAL
		multiColumnPrinter_print(printer, "ENC_UNROLL", "PARTIAL", "", NULL);
		#else
		multiColumnPrinter_print(printer, "ENC_UNROLL", "NONE", "", NULL);
		#endif

		#if DEC_UNROLL == FULL
		multiColumnPrinter_print(printer, "DEC_UNROLL", "FULL", "", NULL);
		#elif DEC_UNROLL  == PARTIAL
		multiColumnPrinter_print(printer, "DEC_UNROLL", "PARTIAL", "", NULL);
		#else
		multiColumnPrinter_print(printer, "DEC_UNROLL", "NONE", "", NULL);
		#endif

		#ifdef ENC_KS_UNROLL
		multiColumnPrinter_print(printer, "ENC_KS_UNROLL", "--def--", "", NULL);
		#else
		multiColumnPrinter_print(printer, "ENC_KS_UNROLL", "--undef--", "", NULL);
		#endif

		#ifdef DEC_KS_UNROLL
		multiColumnPrinter_print(printer, "DEC_KS_UNROLL", "--def--", "", NULL);
		#else
		multiColumnPrinter_print(printer, "DEC_KS_UNROLL", "--undef--", "", NULL);
		#endif

		#ifdef FIXED_TABLES
		multiColumnPrinter_print(printer, "FIXED_TABLES", "--def--", "The tables used by the code are compiled statically into the binary file", NULL);
		#else
		multiColumnPrinter_print(printer, "FIXED_TABLES", "--undef--", "The subroutine aes_init() must be called to compute them before the code is first used", NULL);
		#endif

		#if ENC_ROUND == FOUR_TABLES
		multiColumnPrinter_print(printer, "ENC_ROUND", "FOUR_TABLES", "Set tables for the normal encryption round", NULL);
		#elif ENC_ROUND == ONE_TABLE
		multiColumnPrinter_print(printer, "ENC_ROUND", "ONE_TABLE", "Set tables for the normal encryption round", NULL);
		#else
		multiColumnPrinter_print(printer, "ENC_ROUND", "NO_TABLES", "Set tables for the normal encryption round", NULL);
		#endif

		#if LAST_ENC_ROUND == FOUR_TABLES
		multiColumnPrinter_print(printer, "LAST_ENC_ROUND", "FOUR_TABLES", "Set tables for the last encryption round", NULL);
		#elif LAST_ENC_ROUND == ONE_TABLE
		multiColumnPrinter_print(printer, "LAST_ENC_ROUND", "ONE_TABLE", "Set tables for the last encryption round", NULL);
		#else
		multiColumnPrinter_print(printer, "LAST_ENC_ROUND", "NO_TABLES", "Set tables for the last encryption round", NULL);
		#endif

		#if DEC_ROUND == FOUR_TABLES
		multiColumnPrinter_print(printer, "DEC_ROUND", "FOUR_TABLES", "Set tables for the normal decryption round", NULL);
		#elif DEC_ROUND == ONE_TABLE
		multiColumnPrinter_print(printer, "DEC_ROUND", "ONE_TABLE", "Set tables for the normal decryption round", NULL);
		#else
		multiColumnPrinter_print(printer, "DEC_ROUND", "NO_TABLES", "Set tables for the normal decryption round", NULL);
		#endif

		#if LAST_DEC_ROUND == FOUR_TABLES
		multiColumnPrinter_print(printer, "LAST_DEC_ROUND", "FOUR_TABLES", "Set tables for the last decryption round", NULL);
		#elif LAST_DEC_ROUND == ONE_TABLE
		multiColumnPrinter_print(printer, "LAST_DEC_ROUND", "ONE_TABLE", "Set tables for the last decryption round", NULL);
		#else
		multiColumnPrinter_print(printer, "LAST_DEC_ROUND", "NO_TABLES", "Set tables for the last decryption round", NULL);
		#endif

		#if KEY_SCHED == FOUR_TABLES
		multiColumnPrinter_print(printer, "KEY_SCHED", "FOUR_TABLES", "Set tables for the key schedule", NULL);
		#elif KEY_SCHED == ONE_TABLE
		multiColumnPrinter_print(printer, "KEY_SCHED", "ONE_TABLE", "Set tables for the key schedule", NULL);
		#else
		multiColumnPrinter_print(printer, "KEY_SCHED", "NO_TABLES", "Set tables for the key schedule", NULL);
		#endif

		multiColumnPrinter_delete(printer);
	}
	else{
		printf("ERROR: in %s, unable to create multiColumnPrinter\n", __func__);
	}

	if (aes_init() != EXIT_SUCCESS){
		printf("ERROR: in %s, aes_init failed\n", __func__);
		return 0;
	}
	
	printf("\nPlaintext:      ");
	printBuffer_raw(stdout, (char*)plaintext, 16);

	/* 128 bits AES */
	printf("\nKey 128:        ");
	printBuffer_raw(stdout, (char*)key_128, 16);

	if (aes_encrypt_key128(key_128, &enc_ctx_128) != EXIT_SUCCESS){
		printf("ERROR: in %s, aes_encrypt_key128 failed\n", __func__);
		return 0;
	}

	if (aes_ecb_encrypt(plaintext, ciphertext, 16, &enc_ctx_128) != EXIT_SUCCESS){
		printf("ERROR: in %s, aes_ecb_encrypt failed\n", __func__);
		return 0;
	}

	printf("\nCiphertext 128: ");
	printBuffer_raw(stdout, (char*)ciphertext, 16);

	if (aes_decrypt_key128(key_128, &dec_ctx_128) != EXIT_SUCCESS){
		printf("ERROR: in %s, aes_decrypt_key128 failed\n", __func__);
		return 0;
	}

	if (aes_ecb_decrypt(ciphertext, deciphertext, 16, &dec_ctx_128) != EXIT_SUCCESS){
		printf("ERROR: in %s, aes_ecb_decrypt failed\n", __func__);
		return 0;
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
		return 0;
	}

	if (aes_ecb_encrypt(plaintext, ciphertext, 16, &enc_ctx_192) != EXIT_SUCCESS){
		printf("ERROR: in %s, aes_ecb_encrypt failed\n", __func__);
		return 0;
	}

	printf("\nCiphertext 192: ");
	printBuffer_raw(stdout, (char*)ciphertext, 16);

 	if (aes_decrypt_key192(key_192, &dec_ctx_192) != EXIT_SUCCESS){
		printf("ERROR: in %s, aes_decrypt_key192 failed\n", __func__);
		return 0;
	}

	if (aes_ecb_decrypt(ciphertext, deciphertext, 16, &dec_ctx_192) != EXIT_SUCCESS){
		printf("ERROR: in %s, aes_ecb_decrypt failed\n", __func__);
		return 0;
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
		return 0;
	}

	if (aes_ecb_encrypt(plaintext, ciphertext, 16, &enc_ctx_256) != EXIT_SUCCESS){
		printf("ERROR: in %s, aes_ecb_encrypt failed\n", __func__);
		return 0;
	}

	printf("\nCiphertext 256: ");
	printBuffer_raw(stdout, (char*)ciphertext, 16);

	if (aes_decrypt_key256(key_256, &dec_ctx_256) != EXIT_SUCCESS){
		printf("ERROR: in %s, aes_decrypt_key256 failed\n", __func__);
		return 0;
	}

	if (aes_ecb_decrypt(ciphertext, deciphertext, 16, &dec_ctx_256) != EXIT_SUCCESS){
		printf("ERROR: in %s, aes_ecb_decrypt failed\n", __func__);
		return 0;
	}

	if (memcmp(deciphertext, plaintext, 16) == 0){
		printf("\nRecovery 256:   OK\n");
	}
	else{
		printf("\nRecovery 256:   FAIL\n");
	}

	return 0;
}