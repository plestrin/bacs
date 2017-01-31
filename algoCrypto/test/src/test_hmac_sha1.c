#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "SHA1.h"
#include "mode.h"
#include "printBuffer.h"

#ifdef _WIN32
#include "windowsComp.h"
#endif

int main(void){
	char 				message[] = "Hello I am a test vector for the HMAC SHA1. Since I am 93 bytes wide I lay on several blocks.";
	char 				key[] = "1 4m 4 53cr3t k3y";
	struct sha1State 	sha1_state;
	struct hash 		hash;
	char 				hash_mac[SHA1_HASH_NB_BYTE];

	hash.block_size = SHA1_BLOCK_NB_BYTE;
	hash.hash_size 	= SHA1_HASH_NB_BYTE;
	hash.state 		= &sha1_state;
	hash.func_init 	= (void(*)(void*))sha1_init;
	hash.func_feed 	= (void(*)(void*,void*,uint64_t))sha1_feed;
	hash.func_hash 	= (void(*)(void*,void*))sha1_hash;

	if (hmac(&hash, (uint8_t*)message, (uint8_t*)hash_mac, strlen(message), (uint8_t*)key, strlen(key))){
		printf("ERROR: in %s, the HMAC function failed\n", __func__);
	}
	else{
		printf("Plaintext: \"%s\"\n", message);
		printf("Key:       \"%s\"\n", key);
		printf("SHA1 HMAC: ");
		printBuffer_raw(stdout, hash_mac, SHA1_HASH_NB_BYTE);
		printf("\n");
	}

	return EXIT_SUCCESS;
}
