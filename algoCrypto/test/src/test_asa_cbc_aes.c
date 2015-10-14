#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "AES.h"
#include "mode.h"
#include "printBuffer.h"

static void sas_cbc_aes_encrypt(const unsigned char* subversion_key, const unsigned char* key, const char* pt, size_t pt_length, uint8_t* iv, unsigned char* ct){
	uint32_t round_key[AES_128_NB_DWORD_ROUND_KEY];

	aes128_key_expand_encrypt((const uint32_t*)subversion_key, round_key);
	aes128_encrypt((const uint32_t*)key, round_key, (uint32_t*)iv);

	aes128_key_expand_encrypt((const uint32_t*)key, round_key);
	mode_enc_cbc((blockCipher)aes128_encrypt, AES_BLOCK_NB_BYTE, (const uint8_t*)pt, (uint8_t*)ct, pt_length, round_key, iv);
}

static void sas_cbc_aes_decrypt(const unsigned char* subversion_key, const unsigned char* ct, size_t ct_length, const uint8_t* iv, unsigned char* pt){
	uint32_t round_key[AES_128_NB_DWORD_ROUND_KEY];
	uint32_t key[AES_128_NB_DWORD_KEY];

	aes128_key_expand_decrypt((const uint32_t*)subversion_key, round_key);
	aes128_decrypt((const uint32_t*)iv, round_key, key);

	aes128_key_expand_decrypt(key, round_key);
	mode_dec_cbc((blockCipher)aes128_decrypt, AES_BLOCK_NB_BYTE, (const uint8_t*)ct, (uint8_t*)pt, ct_length, round_key, iv);
}

int main(){
	const char 				plaintext[] = "Hi I am an AES CBC test vector distributed on 4 128-bit blocks!";
	unsigned char 			key_128[AES_128_NB_BYTE_KEY] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
	const unsigned char 	subversion_key[AES_128_NB_BYTE_KEY] = {0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};
	uint8_t 				iv[AES_128_NB_BYTE_KEY];
	unsigned char 			ciphertext[sizeof(plaintext)];
	unsigned char 			deciphertext[sizeof(plaintext)];
	
	printf("Plaintext:      \"%s\"\n", plaintext);
	printf("Key 128:        ");
	printBuffer_raw(stdout, (char*)key_128, AES_128_NB_BYTE_KEY);
	printf("\nSubversion Key: ");
	printBuffer_raw(stdout, (char*)subversion_key, AES_128_NB_BYTE_KEY);

	sas_cbc_aes_encrypt(subversion_key, key_128, plaintext, sizeof(plaintext), iv, ciphertext);

	printf("\nCiphertext CBC: ");
	printBuffer_raw(stdout, (char*)ciphertext, sizeof(plaintext));

	sas_cbc_aes_decrypt(subversion_key, ciphertext, sizeof(ciphertext), iv, deciphertext);

	if (memcmp(deciphertext, plaintext, sizeof(plaintext)) == 0){
		printf("\nRecovery:       OK\n");
	}
	else{
		printf("\nRecovery:       FAIL\n");
	}

	return 0;
}