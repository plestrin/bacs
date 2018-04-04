#include <iostream>

#include <botan/arc4.h>

#include "misc.h"

int main(void){
	unsigned char 	pt[] = "Hello World!";
	unsigned char 	key[] = "Key";
	Botan::byte 	ct[sizeof pt];

	if (botan_patch()){
		std::cout << "ERROR: in " << __func__ << ", unable to patch Botan" << std::endl;
		return EXIT_FAILURE;
	}

	Botan::ARC4 rc4;

	std::cout << "Plaintext:  \"" << pt << "\"" << std::endl;
	std::cout << "Key:        \"" << key << "\"" << std::endl;

	rc4.set_key((Botan::byte*)key, sizeof key - 1);
	rc4.cipher((Botan::byte*)pt, ct, sizeof pt - 1);

	std::cout << "Ciphertext: ";
	print_raw_buffer((Botan::byte*)ct, sizeof pt - 1);
	std::cout << std::endl;

	return EXIT_SUCCESS;
}
