#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <openssl/hmac.h>
#include <openssl/md5.h>

#include "printBuffer.h"

int main(){
	char 			message[] = "Hello I am a test vector for the HMAC MD5. Since I am 92 bytes wide I lay on several blocks.";
	char 			key[] = "1 4m 4 53cr3t k3y";
	char 			hash_mac[16];
	unsigned int 	hash_mac_length = sizeof(hash_mac);

	if (HMAC(EVP_md5(), key, strlen(key), (unsigned char*)message, strlen(message), (unsigned char*)hash_mac, &hash_mac_length) == NULL){
		printf("ERROR: in %s, HMAC failed\n", __func__);
		return EXIT_FAILURE;
	}

	printf("Plaintext: \"%s\"\n", message);
	printf("Key:       \"%s\"\n", key);
	printf("MD5 HMAC:  ");
	printBuffer_raw(stdout, hash_mac, hash_mac_length);
	printf("\n");

	return EXIT_SUCCESS;
}
