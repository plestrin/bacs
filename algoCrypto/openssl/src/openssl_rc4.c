#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <openssl/rc4.h>

#include "printBuffer.h"

int main(void){
	char 		pt[] = "Hello World!";
	char		key[] = "Key";
	char 		ct[sizeof(pt)];
	RC4_KEY 	rc4_key;

	RC4_set_key(&rc4_key, strlen(key), (unsigned char*)key);

	printf("Plaintext:  \"%s\"\n", pt);
	printf("Key:        \"%s\"\n", key);

	RC4(&rc4_key, strlen(pt), (unsigned char*)pt, (unsigned char*)ct);

	printf("Ciphertext: ");
	fprintBuffer_raw(stdout, ct, strlen(pt));
	putchar('\n');

	return EXIT_SUCCESS;
}
