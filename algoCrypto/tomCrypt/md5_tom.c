#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <tomcrypt.h>

#include "../../misc/printBuffer.h"

int main(){
	char 		message[] = "Hello world!";
	uint32_t 	hash[4];
	hash_state 	md;
	
	md5_init(&md);
	md5_process(&md, (unsigned char*)message, strlen(message));
	md5_done(&md, (unsigned char*)hash);

	printf("Plaintext: \"%s\"\n", message);
	printf("MD5 hash:  ");
	printBuffer_raw(stdout, (char*)hash, 16);
	printf("\n");

	return 0;
}
