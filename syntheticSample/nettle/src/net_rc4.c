#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <nettle/arcfour.h>

#include "printBuffer.h"

int main(void){
	char 				pt[] = "Hello World!";
	char				key[] = "Key";
	char 				ct[sizeof pt];
	struct arcfour_ctx 	ctx;

	printf("Plaintext:  \"%s\"\n", pt);
	printf("Key:        \"%s\"\n", key);

	arcfour_set_key(&ctx, strlen(key), (uint8_t*)key);
	arcfour_crypt(&ctx, strlen(pt), (uint8_t*)ct, (uint8_t*)pt);

	printf("Ciphertext: ");
	fprintBuffer_raw(stdout, ct, strlen(pt));
	putchar('\n');

	return EXIT_SUCCESS;
}
