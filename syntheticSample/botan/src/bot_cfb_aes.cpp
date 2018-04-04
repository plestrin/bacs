#include <iostream>

#include <botan/pipe.h>
#include <botan/cfb.h>
#include <botan/aes.h>

#include "misc.h"

int main(void){
	char 		pt[] = "Hi I am an AES CFB test vector distributed on 4 128-bit blocks!";
	Botan::byte key[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
	Botan::byte iv[16] = {0x01, 0xff, 0x83, 0xf2, 0xf9, 0x98, 0xba, 0xa4, 0xda, 0xdc, 0xaa, 0xcc, 0x8e, 0x17, 0xa4, 0x1b};
	Botan::byte ct[sizeof pt];
	Botan::byte	vt[sizeof pt];

	if (botan_patch()){
		std::cout << "ERROR: in " << __func__ << ", unable to patch Botan" << std::endl;
		return EXIT_FAILURE;
	}

	Botan::SymmetricKey 		botan_key(key, sizeof key);
	Botan::InitializationVector botan_iv(iv, sizeof iv);
	Botan::CFB_Encryption* 		enc_cfb_aes;
	Botan::Pipe* 				enc_pipe;
	Botan::CFB_Decryption* 		dec_cfb_aes;
	Botan::Pipe* 				dec_pipe;

	std::cout << "Plaintext:      \"" << pt << "\"" << std::endl;
	std::cout << "IV:             ";
	print_raw_buffer(iv, sizeof iv);
	std::cout << std::endl << "Key 128:        ";
	print_raw_buffer(key, sizeof key);

	enc_cfb_aes = new Botan::CFB_Encryption(new Botan::AES_128, botan_key, botan_iv);
	enc_pipe = new Botan::Pipe(enc_cfb_aes, NULL, NULL);

	enc_pipe->process_msg((Botan::byte*)pt, sizeof pt);
	if (enc_pipe->read(ct, sizeof pt) != sizeof pt){
		std::cout << std::endl << "ERROR: in " << __func__ << ", the number of byte read from the pipe is incorrect" << std::endl;
		return EXIT_FAILURE;
	}

	delete(enc_pipe);

	dec_cfb_aes = new Botan::CFB_Decryption(new Botan::AES_128, botan_key, botan_iv);
	dec_pipe = new Botan::Pipe(dec_cfb_aes, NULL, NULL);

	dec_pipe->process_msg(ct, sizeof pt);
	if (dec_pipe->read(vt, sizeof pt) != sizeof pt){
		std::cout << std::endl << "ERROR: in " << __func__ << ", the number of byte read from the pipe is incorrect" << std::endl;
		return EXIT_FAILURE;
	}

	delete(dec_pipe);

	std::cout << std::endl << "Ciphertext CFB: ";
	print_raw_buffer(ct, sizeof ct);

	if (memcmp(pt, vt, sizeof pt)){
		std::cout << std::endl << "Recovery:       FAIL" << std::endl;
		return EXIT_FAILURE;
	}

	std::cout << std::endl << "Recovery:       OK" << std::endl;

	return EXIT_SUCCESS;
}
