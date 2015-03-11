#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "TEA.h"
#include "mode.h"
#include "printBuffer.h"

#ifdef WIN32
#include "windowsComp.h"
#endif

int main(){
	char 			plaintext[] = "Hi I am an XTEA CTR test vector distributed on 8 64-bit blocks!";
	unsigned char 	key[TEA_KEY_NB_BYTE] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
	unsigned char 	iv[TEA_BLOCK_NB_BYTE] = {0x01, 0xff, 0x83, 0xf2, 0xf9, 0x98, 0xba, 0xa4};
	unsigned char 	ciphertext[sizeof(plaintext)];
	unsigned char 	deciphertext[sizeof(plaintext)];
	
	printf("Plaintext:      \"%s\"\n", plaintext);
	printf("IV:             ");
	printBuffer_raw(stdout, (char*)iv, TEA_BLOCK_NB_BYTE);
	printf("\nKey:            ");
	printBuffer_raw(stdout, (char*)key, TEA_KEY_NB_BYTE);

	mode_enc_ctr((blockCipher)xtea_encrypt, TEA_BLOCK_NB_BYTE, (uint8_t*)plaintext, (uint8_t*)ciphertext, sizeof(plaintext), (void*)key, iv);

	printf("\nCiphertext CTR: ");
	printBuffer_raw(stdout, (char*)ciphertext, sizeof(plaintext));

 	mode_dec_ctr((blockCipher)xtea_encrypt, TEA_BLOCK_NB_BYTE, (uint8_t*)ciphertext, (uint8_t*)deciphertext, sizeof(plaintext), (void*)key, iv);

	if (memcmp(deciphertext, plaintext, sizeof(plaintext)) == 0){
		printf("\nRecovery:       OK\n");
	}
	else{
		printf("\nRecovery:       FAIL\n");
	}

	return 0;
}