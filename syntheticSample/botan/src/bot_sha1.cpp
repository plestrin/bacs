#include <iostream>

#include <botan/sha160.h>

#include "misc.h"

int main(void){
	char 		message[] = "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu";
	Botan::byte hash[20];

	if (botan_patch()){
		std::cout << "ERROR: in " << __func__ << ", unable to patch Botan" << std::endl;
		return EXIT_FAILURE;
	}

	Botan::SHA_160 sha1;

	std::cout << "Plaintext: \"" << message << "\"" << std::endl;

	sha1.update((Botan::byte*)message, strlen(message));
	sha1.final(hash);

	std::cout << "SHA1 hash: ";
	print_raw_buffer(hash, sizeof hash);
	std::cout << std::endl;

	return EXIT_SUCCESS;
}
