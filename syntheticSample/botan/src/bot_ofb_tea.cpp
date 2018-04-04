#include <iostream>

#include <botan/filters.h>
#include <botan/ofb.h>
#include <botan/xtea.h>

#include "misc.h"

int main(void){
	char 		pt[] = "Hi I am an XTEA OFB test vector distributed on 8 64-bit blocks!";
	Botan::byte key[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
	Botan::byte iv[8] = {0x01, 0xff, 0x83, 0xf2, 0xf9, 0x98, 0xba, 0xa4};
	Botan::byte ct[sizeof pt];
	Botan::byte	vt[sizeof pt];

	if (botan_patch()){
		std::cout << "ERROR: in " << __func__ << ", unable to patch Botan" << std::endl;
		return EXIT_FAILURE;
	}

	Botan::XTEA* 				tea;
	Botan::StreamCipher_Filter* enc_ofb_tea;
	Botan::Pipe* 				enc_pipe;
	Botan::StreamCipher_Filter* dec_ofb_tea;
	Botan::Pipe* 				dec_pipe;

	std::cout << "Plaintext:      \"" << pt << "\"" << std::endl;
	std::cout << "IV:             ";
	print_raw_buffer(iv, sizeof iv);
	std::cout << std::endl << "Key:            ";
	print_raw_buffer(key, sizeof key);

	swap_endianness((Botan::byte*)pt, sizeof pt);
	swap_endianness(key, sizeof key);
	swap_endianness(iv, sizeof iv);

	Botan::SymmetricKey 		botan_key(key, sizeof key);
	Botan::InitializationVector botan_iv(iv, sizeof iv);

	tea = new Botan::XTEA();
	enc_ofb_tea = new Botan::StreamCipher_Filter(new Botan::OFB(tea->clone()), botan_key);
	enc_ofb_tea->set_iv(botan_iv);
	enc_pipe = new Botan::Pipe(enc_ofb_tea, NULL, NULL);

	enc_pipe->process_msg((Botan::byte*)pt, sizeof pt);
	if (enc_pipe->read(ct, sizeof pt) != sizeof pt){
		std::cout << std::endl << "ERROR: the number of byte read from the pipe is incorrect" << std::endl;
		return EXIT_FAILURE;
	}

	delete(enc_pipe);

	dec_ofb_tea = new Botan::StreamCipher_Filter(new Botan::OFB(tea->clone()), botan_key);
	dec_ofb_tea->set_iv(botan_iv);
	dec_pipe = new Botan::Pipe(dec_ofb_tea, NULL, NULL);

	dec_pipe->process_msg(ct, sizeof pt);
	if (dec_pipe->read(vt, sizeof pt) != sizeof pt){
		std::cout << std::endl << "ERROR: the number of byte read from the pipe is incorrect" << std::endl;
		return EXIT_FAILURE;
	}

	delete(dec_pipe);

	delete(tea);

	swap_endianness(ct, sizeof pt);
	std::cout << std::endl << "Ciphertext OFB: ";
	print_raw_buffer(ct, sizeof pt);

	if (memcmp(pt, vt, sizeof pt)){
		std::cout << std::endl << "Recovery:       FAIL" << std::endl;
		return EXIT_FAILURE;
	}

	std::cout << std::endl << "Recovery:       OK" << std::endl;

	return EXIT_SUCCESS;
}
