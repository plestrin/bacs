#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <nettle/aes.h>
#include <nettle/ctr.h>

#include "printBuffer.h"

#define CTR_SET_KEY(ctx_, key) aes_set_encrypt_key(&((ctx_)->ctx), 16, key)

int main(){
	char 	plaintext[] = "Hi I am an AES CTR test vector distributed on 4 128-bit blocks!";
	uint8_t key[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
	uint8_t iv[16] = {0x01, 0xff, 0x83, 0xf2, 0xf9, 0x98, 0xba, 0xa4, 0xda, 0xdc, 0xaa, 0xcc, 0x8e, 0x17, 0xa4, 0x1b};
	uint8_t ciphertext[sizeof(plaintext)];
	uint8_t deciphertext[sizeof(plaintext)];
	struct CTR_CTX(struct aes_ctx, AES_BLOCK_SIZE) ctx;

	printf("Plaintext:      \"%s\"\n", plaintext);
	printf("IV:             ");
	fprintBuffer_raw(stdout, (char*)iv, sizeof(iv));
	printf("\nKey 128:        ");
	fprintBuffer_raw(stdout, (char*)key, sizeof(key));

	CTR_SET_COUNTER(&ctx, iv);
	CTR_SET_KEY(&ctx, key);
	CTR_CRYPT(&ctx, aes_encrypt, sizeof(plaintext), ciphertext, (uint8_t*)plaintext);

	CTR_SET_COUNTER(&ctx, iv);
	CTR_SET_KEY(&ctx, key);
	CTR_CRYPT(&ctx, aes_encrypt, sizeof(ciphertext), deciphertext, ciphertext);

	printf("\nCiphertext CTR: ");
	fprintBuffer_raw(stdout, (char*)ciphertext, sizeof(plaintext));

	if (memcmp(deciphertext, plaintext, sizeof(plaintext)) == 0){
		printf("\nRecovery:       OK\n");
	}
	else{
		printf("\nRecovery:       FAIL\n");
	}
	return EXIT_SUCCESS;
}
