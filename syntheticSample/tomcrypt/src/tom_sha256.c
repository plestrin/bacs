#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <tomcrypt.h>

#include "printBuffer.h"

int main(void){
	char 		message[] = "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu";
	uint32_t 	hash[8];
	hash_state 	md;

	sha256_init(&md);
	sha256_process(&md, (unsigned char*)message, strlen(message));
	sha256_done(&md, (unsigned char*)hash);

	printf("Plaintext:   \"%s\"\n", message);
	printf("SHA256 hash: ");
	fprintBuffer_raw(stdout, (char*)hash, sizeof(hash));
	printf("\n");

	return 0;
}
