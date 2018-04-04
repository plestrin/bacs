#include <iostream>

#include <botan/ctr.h>
#include <botan/aes.h>

#include "misc.h"

int main(void){
	char 			pt[] = "Hi I am an AES CTR test vector distributed on 4 128-bit blocks!";
	Botan::byte 	key[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
	Botan::byte 	iv[16] = {0x01, 0xff, 0x83, 0xf2, 0xf9, 0x98, 0xba, 0xa4, 0xda, 0xdc, 0xaa, 0xcc, 0x8e, 0x17, 0xa4, 0x1b};
	Botan::byte 	ct[sizeof pt];
	Botan::byte		vt[sizeof pt];
	Botan::CTR_BE* 	ctr_aes;

	if (botan_patch()){
		std::cout << "ERROR: in " << __func__ << ", unable to patch Botan" << std::endl;
		return EXIT_FAILURE;
	}

	std::cout << "Plaintext:      \"" << pt << "\"" << std::endl;
	std::cout << "IV:             ";
	print_raw_buffer(iv, sizeof iv);
	std::cout << std::endl << "Key 128:        ";
	print_raw_buffer(key, sizeof key);

	ctr_aes = new Botan::CTR_BE(new Botan::AES_128);
	ctr_aes->set_key(key, sizeof key);
	ctr_aes->set_iv(iv, sizeof iv);
	ctr_aes->cipher((Botan::byte*)pt, ct, sizeof pt);

	ctr_aes->clear();

	ctr_aes->set_key(key, sizeof key);
	ctr_aes->set_iv(iv, sizeof iv);
	ctr_aes->cipher(ct, vt, sizeof pt);

	delete(ctr_aes);

	std::cout << std::endl << "Ciphertext CTR: ";
	print_raw_buffer(ct, sizeof ct);

	if (memcmp(pt, vt, sizeof pt)){
		std::cout << std::endl << "Recovery:       FAIL" << std::endl;
		return EXIT_FAILURE;
	}

	std::cout << std::endl << "Recovery:       OK" << std::endl;

	return EXIT_SUCCESS;
}
