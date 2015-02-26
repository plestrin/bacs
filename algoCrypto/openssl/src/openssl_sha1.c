#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <openssl/sha.h>

#include "printBuffer.h"

int main(){
	char 		message[] = "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu";
	uint32_t 	hash[5];
	SHA_CTX 	sha1_context;

	SHA1_Init(&sha1_context);
	SHA1_Update(&sha1_context, message, strlen(message));
	SHA1_Final((unsigned char*)hash, &sha1_context);

	printf("Plaintext: \"%s\"\n", message);
	printf("SHA1 hash: ");
	printBuffer_raw(stdout, (char*)hash, sizeof(hash));
	printf("\n");

	return 0;
}