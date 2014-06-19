#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <openssl/md5.h>

#include "printBuffer.h"

int main(){
	char 		message[] = "12345678901234567890123456789012345678901234567890123456789012345678901234567890";
	uint32_t 	hash[4];
	MD5_CTX 	md5_context;

	MD5_Init(&md5_context);
	MD5_Update(&md5_context, message, strlen(message));
	MD5_Final((unsigned char*)hash, &md5_context);

	printf("Plaintext: \"%s\"\n", message);
	printf("MD5 hash:  ");
	printBuffer_raw(stdout, (char*)hash, sizeof(hash));
	printf("\n");

	return 0;
}