#include <iostream>

#include <botan/des.h>

#include "misc.h"

int main(void){
	Botan::byte key[8] = {0x75, 0x29, 0x79, 0x38, 0x75, 0x92, 0xcb, 0x70};
	Botan::byte pt[8] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
	Botan::byte ct[sizeof pt];
	Botan::byte vt[sizeof pt];

	if (botan_patch()){
		std::cout << "ERROR: in " << __func__ << ", unable to patch Botan" << std::endl;
		return EXIT_FAILURE;
	}

	Botan::DES des;

	std::cout << "Plaintext:  ";
	print_raw_buffer(pt, sizeof pt);

	std::cout << std::endl << "Key:        ";
	print_raw_buffer(key, sizeof key);

	des.set_key(key, sizeof key);
	des.encrypt_n(pt, ct, 1);
	des.decrypt_n(ct, vt, 1);

	std::cout << std::endl << "Ciphertext: ";
	print_raw_buffer(ct, sizeof ct);

	if (memcmp(pt, vt, sizeof pt)){
		std::cout << std::endl << "Recovery:   FAIL" << std::endl;
		return EXIT_FAILURE;
	}

	std::cout << std::endl << "Recovery:   OK" << std::endl;

	return EXIT_SUCCESS;
}
