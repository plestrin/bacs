#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <tomcrypt.h>

#include "printBuffer.h"

int main(void){
	char 		pt[] = "Hello World!";
	char		key[] = "Key";
	char 		ct[sizeof(pt)];
	prng_state 	prng;

	printf("Plaintext:  \"%s\"\n", pt);
	printf("Key:        \"%s\"\n", key);

	memcpy(ct, pt, sizeof(pt));

	rc4_start(&prng);
	rc4_add_entropy((unsigned char*)key, strlen(key), &prng);
	rc4_ready(&prng);
	rc4_read((unsigned char*)ct, strlen(pt), &prng);

	printf("Ciphertext: ");
	fprintBuffer_raw(stdout, ct, strlen(pt));
	puts("\nCheck:      OK");

	return EXIT_SUCCESS;
}
