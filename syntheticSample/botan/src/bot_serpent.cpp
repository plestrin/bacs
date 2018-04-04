#include <iostream>

#include <botan/serpent.h>

#include "misc.h"

int main(void){
	Botan::byte key[24] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17};

	Botan::byte pt[16] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
	Botan::byte ct[sizeof pt];
	Botan::byte vt[sizeof pt];

	if (botan_patch()){
		std::cout << "ERROR: in " << __func__ << ", unable to patch Botan" << std::endl;
		return EXIT_FAILURE;
	}

	Botan::Serpent serpent;

	std::cout << "Plaintext:  ";
	print_raw_buffer(pt, sizeof pt);

	std::cout << std::endl << "Key:        ";
	print_raw_buffer(key, sizeof key);

	serpent.set_key(key, sizeof key);
	serpent.encrypt_n(pt, ct, 1);
	serpent.decrypt_n(ct, vt, 1);

	std::cout << std::endl << "Ciphertext: ";
	print_raw_buffer(ct, sizeof ct);

	if (memcmp(pt, vt, sizeof pt)){
		std::cout << std::endl << "Recovery:   FAIL" << std::endl;
		return EXIT_FAILURE;
	}

	std::cout << std::endl << "Recovery:   OK" << std::endl;

	return EXIT_SUCCESS;
}
