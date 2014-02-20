#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <tomcrypt.h>

#include "printBuffer.h"

int main(){
	char 		message[] = "12345678901234567890123456789012345678901234567890123456789012345678901234567890";
	uint32_t 	hash[4];
	hash_state 	md;
	
	md5_init(&md);
	md5_process(&md, (unsigned char*)message, strlen(message));
	md5_done(&md, (unsigned char*)hash);

	printf("Plaintext: \"%s\"\n", message);
	printf("MD5 hash:  ");
	printBuffer_raw(stdout, (char*)hash, sizeof(hash));
	printf("\n");

	return 0;
}
