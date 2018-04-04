#include <iostream>

#include <botan/twofish.h>

#include "misc.h"

int main(void){
	Botan::byte key_128[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
	Botan::byte key_192[24] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17};
	Botan::byte key_256[32] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};

	Botan::byte pt[16] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
	Botan::byte ct[sizeof pt];
	Botan::byte vt[sizeof pt];

	if (botan_patch()){
		std::cout << "ERROR: in " << __func__ << ", unable to patch Botan" << std::endl;
		return EXIT_FAILURE;
	}

	Botan::Twofish twofish128;
	Botan::Twofish twofish192;
	Botan::Twofish twofish256;

	std::cout << "Plaintext:      ";
	print_raw_buffer(pt, sizeof pt);

	/* 128 bits TWOFISH */
	std::cout << std::endl << "Key 128:        ";
	print_raw_buffer(key_128, sizeof key_128);

	twofish128.set_key(key_128, sizeof key_128);
	twofish128.encrypt_n(pt, ct, 1);
	twofish128.decrypt_n(ct, vt, 1);

	std::cout << std::endl << "Ciphertext 128: ";
	print_raw_buffer(ct, sizeof ct);

	if (memcmp(pt, vt, sizeof pt)){
		std::cout << std::endl << "Recovery 128:   FAIL" << std::endl;
		return EXIT_FAILURE;
	}

	std::cout << std::endl << "Recovery 128:   OK" << std::endl;

	/* 192 bits TWOFISH */
	std::cout << "Key 192:        ";
	print_raw_buffer(key_192, sizeof key_192);

	twofish192.set_key(key_192, sizeof key_192);
	twofish192.encrypt_n(pt, ct, 1);
	twofish192.decrypt_n(ct, vt, 1);

	std::cout << std::endl << "Ciphertext 192: ";
	print_raw_buffer(ct, sizeof ct);

	if (memcmp(pt, vt, sizeof pt)){
		std::cout << std::endl << "Recovery 192:   FAIL" << std::endl;
		return EXIT_FAILURE;
	}

	std::cout << std::endl << "Recovery 192:   OK" << std::endl;

	/* 256 bits TWOFISH */
	std::cout << "Key 256:        ";
	print_raw_buffer(key_256, sizeof key_256);

	twofish256.set_key(key_256, sizeof key_256);
	twofish256.encrypt_n(pt, ct, 1);
	twofish256.decrypt_n(ct, vt, 1);

	std::cout << std::endl << "Ciphertext 256: ";
	print_raw_buffer(ct, sizeof ct);

	if (memcmp(pt, vt, sizeof pt)){
		std::cout << std::endl << "Recovery 256:   FAIL" << std::endl;
		return EXIT_FAILURE;
	}

	std::cout << std::endl << "Recovery 256:   OK" << std::endl;

	return EXIT_SUCCESS;
}
