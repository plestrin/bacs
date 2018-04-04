#include <iostream>

#include <botan/md5.h>
#include <botan/hmac.h>

#include "misc.h"

int main(void){
	char 			message[] = "Hello I am a test vector for the HMAC MD5. Since I am 92 bytes wide I lay on several blocks.";
	char 			key[] = "1 4m 4 53cr3t k3y";
	Botan::byte 	hash_mac[16];
	Botan::HMAC* 	hmac;

	if (botan_patch()){
		std::cout << "ERROR: in " << __func__ << ", unable to patch Botan" << std::endl;
		return EXIT_FAILURE;
	}

	hmac = new Botan::HMAC(new Botan::MD5());

	std::cout << "Plaintext: \"" << message << "\"" << std::endl;
	std::cout << "Key:       \"" << key << "\"" << std::endl;

	hmac->set_key((Botan::byte*)key, strlen(key));
	hmac->update((Botan::byte*)message, strlen(message));
	hmac->final(hash_mac);

	std::cout << "MD5 HMAC:  ";
	print_raw_buffer(hash_mac, sizeof(hash_mac));
	std::cout << std::endl;

	delete(hmac);

	return EXIT_SUCCESS;
}
