#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "SHA1.h"
#include "printBuffer.h"

#ifdef _WIN32
#include "windowsComp.h"
#endif

/* This test vector is taken from http://www.di-mgt.com.au/sha_testvectors.html */

int main(void){
	const char 			message[] = "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu";
	uint32_t 			hash[SHA1_HASH_NB_DWORD];
	struct sha1State 	state;

	sha1_init(&state);
	sha1_feed(&state, (const uint32_t*)message, strlen(message));
	sha1_hash(&state, hash);

	printf("Plaintext: \"%s\"\n", message);
	printf("SHA1 hash: ");
	fprintBuffer_raw(stdout, (char*)hash, SHA1_HASH_NB_BYTE);
	putchar('\n');

	return EXIT_SUCCESS;
}
