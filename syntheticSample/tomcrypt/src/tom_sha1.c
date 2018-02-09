#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <tomcrypt.h>

#include "printBuffer.h"

int main(void){
	char 		message[] = "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu";
	uint32_t 	hash[5];
	hash_state 	md;

	sha1_init(&md);
	sha1_process(&md, (unsigned char*)message, strlen(message));
	sha1_done(&md, (unsigned char*)hash);

	printf("Plaintext: \"%s\"\n", message);
	printf("SHA1 hash: ");
	fprintBuffer_raw(stdout, (char*)hash, sizeof(hash));
	printf("\n");

	return 0;
}
