#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <nettle/hmac.h>

#include "printBuffer.h"

int main(void){
	char 				message[] = "Hello I am a test vector for the HMAC MD5. Since I am 92 bytes wide I lay on several blocks.";
	char 				key[] = "1 4m 4 53cr3t k3y";
	struct hmac_md5_ctx ctx;
	char 				hash_mac[16];
	unsigned long 		hash_mac_length = sizeof hash_mac;

	hmac_md5_set_key(&ctx, strlen(key), (uint8_t*)key);
	hmac_md5_update(&ctx, strlen(message), (uint8_t*)message);
	hmac_md5_digest(&ctx, hash_mac_length, (uint8_t*)hash_mac);

	printf("Plaintext: \"%s\"\n", message);
	printf("Key:       \"%s\"\n", key);
	printf("MD5 HMAC:  ");
	fprintBuffer_raw(stdout, hash_mac, hash_mac_length);
	printf("\n");

	return EXIT_SUCCESS;
}
