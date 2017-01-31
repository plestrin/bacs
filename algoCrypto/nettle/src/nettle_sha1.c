#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <nettle/sha1.h>

#include "printBuffer.h"

int main(){
	char 			message[] = "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu";
	uint32_t 		hash[5];
	struct sha1_ctx ctx;

	sha1_init(&ctx);
	sha1_update(&ctx, strlen(message), (uint8_t*)message);
	sha1_digest(&ctx, sizeof(hash), (uint8_t*)hash);

	printf("Plaintext: \"%s\"\n", message);
	printf("SHA1 hash: ");
	printBuffer_raw(stdout, (char*)hash, sizeof(hash));
	printf("\n");

	return EXIT_SUCCESS;
}
