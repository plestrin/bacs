#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "RC4.h"
#include "printBuffer.h"

#ifdef _WIN32
#include "windowsComp.h"
#endif

int main(void){
	char 	pt[] = "Hello World!";
	char	key[] = "Key";
	char 	ct[sizeof(pt)];
	size_t 	plaintext_length = strlen(pt);
	size_t 	key_length = strlen(key);

	printf("Plaintext:  \"%s\"\n", pt);
	printf("Key:        \"%s\"\n", key);

	rc4((uint8_t*)pt, plaintext_length, (uint8_t*)key, key_length, (uint8_t*)ct);

	printf("Ciphertext: ");
	fprintBuffer_raw(stdout, ct, plaintext_length);
	putchar('\n');

	return EXIT_SUCCESS;
}
