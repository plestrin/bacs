#include <iostream>

#include <botan/md5.h>

#include "misc.h"

int main(void){
	char 		message[] = "12345678901234567890123456789012345678901234567890123456789012345678901234567890";
	Botan::byte hash[16];

	if (botan_patch()){
		std::cout << "ERROR: in " << __func__ << ", unable to patch Botan" << std::endl;
		return EXIT_FAILURE;
	}

	Botan::MD5 md5;

	std::cout << "Plaintext: \"" << message << "\"" << std::endl;

	md5.update((Botan::byte*)message, strlen(message));
	md5.final(hash);

	std::cout << "MD5 hash:  ";
	print_raw_buffer(hash, sizeof hash);
	std::cout << std::endl;

	return EXIT_SUCCESS;
}
