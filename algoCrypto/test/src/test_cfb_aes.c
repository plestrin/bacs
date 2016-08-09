#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "AES.h"
#include "mode.h"
#include "printBuffer.h"

#ifdef _WIN32
#include "windowsComp.h"
#endif

int main(void){
	char 			plaintext[] = "Hi I am an AES CFB test vector distributed on 4 128-bit blocks!";
	unsigned char 	key_128[AES_128_NB_BYTE_KEY] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
	unsigned char 	iv[AES_128_NB_BYTE_KEY] = {0x01, 0xff, 0x83, 0xf2, 0xf9, 0x98, 0xba, 0xa4, 0xda, 0xdc, 0xaa, 0xcc, 0x8e, 0x17, 0xa4, 0x1b};
	unsigned char 	round_key_128[AES_128_NB_BYTE_ROUND_KEY];
	unsigned char 	ciphertext[sizeof(plaintext)];
	unsigned char 	deciphertext[sizeof(plaintext)];
	
	printf("Plaintext:      \"%s\"\n", plaintext);
	printf("IV:             ");
	printBuffer_raw(stdout, (char*)iv, AES_128_NB_BYTE_KEY);
	printf("\nKey 128:        ");
	printBuffer_raw(stdout, (char*)key_128, AES_128_NB_BYTE_KEY);

	aes128_key_expand_encrypt((uint32_t*)key_128, (uint32_t*)round_key_128);
	mode_enc_cfb((blockCipher)aes128_encrypt, AES_BLOCK_NB_BYTE, (uint8_t*)plaintext, (uint8_t*)ciphertext, sizeof(plaintext), (void*)round_key_128, (uint8_t*)iv);

	printf("\nCiphertext CFB: ");
	printBuffer_raw(stdout, (char*)ciphertext, sizeof(plaintext));

 	mode_dec_cfb((blockCipher)aes128_encrypt, AES_BLOCK_NB_BYTE, (uint8_t*)ciphertext, (uint8_t*)deciphertext, sizeof(plaintext), (void*)round_key_128, (uint8_t*)iv);

	if (memcmp(deciphertext, plaintext, sizeof(plaintext)) == 0){
		printf("\nRecovery:       OK\n");
	}
	else{
		printf("\nRecovery:       FAIL\n");
	}

	return EXIT_SUCCESS;
}