#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <tomcrypt.h>

#include "printBuffer.h"

int main(void){
	char 			message[] = "Hello I am a test vector for the HMAC SHA1. Since I am 93 bytes wide I lay on several blocks.";
	char 			key[] = "1 4m 4 53cr3t k3y";
	hmac_state 		hmac;
	char 			hash_mac[20];
	unsigned long 	hash_mac_length = sizeof(hash_mac);
	int 			err;

	if (register_hash(&sha1_desc) == -1) {
		printf("Error: in %s, unable to register hash\n", __func__);
		return 0;
	}

	if (CRYPT_OK != (err = hmac_init(&hmac, find_hash("sha1"), (unsigned char*)key, strlen(key)))){
		printf("ERROR: in %s, hmac_init failed %d\n", __func__, err);
		return 0;
	}
	if (CRYPT_OK != (err = hmac_process(&hmac, (unsigned char*)message, strlen(message)))){
		printf("ERROR: in %s, hmac_process failed %d\n", __func__, err);
		return 0;
	}
	if (CRYPT_OK != (err = hmac_done(&hmac, (unsigned char*)hash_mac, &hash_mac_length))){
		printf("ERROR: in %s, hmac_done failed %d\n", __func__, err);
		return 0;
	}

	printf("Plaintext: \"%s\"\n", message);
	printf("Key:       \"%s\"\n", key);
	printf("SHA1 HMAC: ");
	fprintBuffer_raw(stdout, hash_mac, hash_mac_length);
	printf("\n");

	return 0;
}
