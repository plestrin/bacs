#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <nettle/md5.h>

#include "printBuffer.h"

int main(){
	char 			message[] = "12345678901234567890123456789012345678901234567890123456789012345678901234567890";
	uint32_t 		hash[4];
	struct md5_ctx 	ctx;

	md5_init(&ctx);
	md5_update(&ctx, strlen(message), (uint8_t*)message);
	md5_digest(&ctx, sizeof(hash), (uint8_t*)hash);

	printf("Plaintext: \"%s\"\n", message);
	printf("MD5 hash:  ");
	fprintBuffer_raw(stdout, (char*)hash, sizeof(hash));
	printf("\n");

	return EXIT_SUCCESS;
}
