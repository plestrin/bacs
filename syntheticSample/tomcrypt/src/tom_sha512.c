#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <tomcrypt.h>

#include "printBuffer.h"

int main(){
	char 		message[] = "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstuabcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu";
	uint32_t 	hash[16];
	hash_state 	md;

	sha512_init(&md);
	sha512_process(&md, (unsigned char*)message, strlen(message));
	sha512_done(&md, (unsigned char*)hash);

	printf("Plaintext:   \"%s\"\n", message);
	printf("SHA512 hash: ");
	fprintBuffer_raw(stdout, (char*)hash, sizeof(hash));
	printf("\n");

	return 0;
}
