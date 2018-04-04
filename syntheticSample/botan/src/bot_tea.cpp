#include <iostream>

#include <botan/tea.h>
#include <botan/xtea.h>

#include "misc.h"

int main(void){
	Botan::byte key[16]	= {0x12, 0x45, 0xf0, 0x6a, 0x45, 0x89, 0xfe, 0x60, 0x50, 0xAA, 0x78, 0x59, 0xf5, 0x69, 0x41, 0xbb};
	Botan::byte pt[8] 	= {0x45, 0xb7, 0x28, 0xba, 0xd7, 0x8f, 0x1a, 0x1f};
	Botan::byte ct[sizeof pt];
	Botan::byte vt[sizeof pt];

	if (botan_patch()){
		std::cout << "ERROR: in " << __func__ << ", unable to patch Botan" << std::endl;
		return EXIT_FAILURE;
	}

	Botan::TEA 		tea;
	Botan::XTEA 	xtea;

	std::cout << "Plaintext:       ";
	print_raw_buffer(pt, sizeof pt);
	std::cout << std::endl << "Key:             ";
	print_raw_buffer(key, sizeof key);

	/* Botan TEA seems to be Big endian */
	swap_endianness(pt, sizeof pt);
	swap_endianness(key, sizeof key);

	tea.set_key(key, sizeof key);
	tea.encrypt_n(pt, ct, 1);
	tea.decrypt_n(ct, vt, 1);

	std::cout << std::endl << "Ciphertext TEA:  ";
	swap_endianness(ct, sizeof ct);
	print_raw_buffer(ct, sizeof ct);

	if (memcmp(pt, vt, sizeof pt)){
		std::cout << std::endl << "Recovery:        FAIL" << std::endl;
		return EXIT_FAILURE;
	}
	std::cout << std::endl << "Recovery:        OK" << std::endl;

	xtea.set_key(key, sizeof key);
	xtea.encrypt_n(pt, ct, 1);
	xtea.decrypt_n(ct, vt, 1);

	std::cout << "Ciphertext XTEA: ";
	swap_endianness(ct, sizeof ct);
	print_raw_buffer(ct, sizeof ct);

	if (memcmp(pt, vt, sizeof pt)){
		std::cout << std::endl << "Recovery:        FAIL" << std::endl;
		return EXIT_FAILURE;
	}

	std::cout << std::endl << "Recovery:        OK" << std::endl;

	return EXIT_SUCCESS;
}
