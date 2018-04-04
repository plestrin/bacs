#include <iostream>

#include <botan/pipe.h>
#include <botan/ecb.h>
#include <botan/xtea.h>

#include "misc.h"

int main(void){
	char 		pt[] = "Hi I am an XTEA ECB test vector distributed on 8 64-bit blocks!";
	Botan::byte key[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
	Botan::byte ct[sizeof pt];
	Botan::byte	vt[sizeof pt];

	if (botan_patch()){
		std::cout << "ERROR: in " << __func__ << ", unable to patch Botan" << std::endl;
		return EXIT_FAILURE;
	}

	Botan::ECB_Encryption* 	enc_ecb_tea;
	Botan::Pipe* 			enc_pipe;
	Botan::ECB_Decryption* 	dec_ecb_tea;
	Botan::Pipe* 			dec_pipe;

	std::cout << "Plaintext:      \"" << pt << "\"" << std::endl;
	std::cout << "Key:            ";
	print_raw_buffer(key, sizeof key);

	swap_endianness((Botan::byte*)pt, sizeof pt);
	swap_endianness(key, sizeof key);

	Botan::SymmetricKey botan_key(key, sizeof key);

	enc_ecb_tea = new Botan::ECB_Encryption(new Botan::XTEA, new Botan::Null_Padding, botan_key);
	enc_pipe = new Botan::Pipe(enc_ecb_tea, NULL, NULL);

	enc_pipe->process_msg((Botan::byte*)pt, sizeof pt);
	if (enc_pipe->read(ct, sizeof pt) != sizeof pt){
		std::cout << std::endl << "ERROR: in " << __func__ << ", the number of byte read from the pipe is incorrect" << std::endl;
		return EXIT_FAILURE;
	}

	delete(enc_pipe);

	dec_ecb_tea = new Botan::ECB_Decryption(new Botan::XTEA, new Botan::Null_Padding, botan_key);
	dec_pipe = new Botan::Pipe(dec_ecb_tea, NULL, NULL);

	dec_pipe->process_msg(ct, sizeof pt);
	if (dec_pipe->read(vt, sizeof pt) != sizeof pt){
		std::cout << std::endl << "ERROR: in " << __func__ << ", the number of byte read from the pipe is incorrect" << std::endl;
		return EXIT_FAILURE;
	}

	delete(dec_pipe);

	swap_endianness(ct, sizeof pt);
	std::cout << std::endl << "Ciphertext ECB: ";
	print_raw_buffer(ct, sizeof pt);

	if (memcmp(pt, vt, sizeof pt)){
		std::cout << std::endl << "Recovery:       FAIL" << std::endl;
		return EXIT_FAILURE;
	}

	std::cout << std::endl << "Recovery:       OK" << std::endl;

	return EXIT_SUCCESS;
}
