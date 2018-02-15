#include <iostream>
#include <iomanip>

#include <cryptopp/modes.h>
#include <cryptopp/twofish.h>

static void print_raw_buffer(byte* buffer, size_t buffer_length){
	size_t i;

	for (i = 0; i < buffer_length; i++){
		if (buffer[i] & 0xf0){
			std::cout << std::hex << (buffer[i] & 0xff);
		}
		else{
			std::cout << "0" << std::hex << (buffer[i] & 0xff);
		}
	}
}

int main(void){
	byte	key128[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
	byte	key192[24] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17};
	byte	key256[32] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};
	byte	pt[16] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
	byte	ct[sizeof pt];
	byte	vt[sizeof ct];

	/* TWOFISH 128 */
	CryptoPP::ECB_Mode<CryptoPP::Twofish>::Encryption ecb_enc_twofish128(key128, sizeof key128);
	ecb_enc_twofish128.ProcessData(ct, pt, sizeof pt);

	std::cout << "Plaintext:      ";
	print_raw_buffer(pt, sizeof pt);
	std::cout << std::endl << "Key 128:        ";
	print_raw_buffer(key128, sizeof key128);
	std::cout << std::endl << "Ciphertext 128: ";
	print_raw_buffer(ct, sizeof ct);

	CryptoPP::ECB_Mode<CryptoPP::Twofish>::Decryption ecb_dec_twofish128(key128, sizeof key128);
	ecb_dec_twofish128.ProcessData(vt, ct, sizeof ct);

	if (memcmp(pt, vt, sizeof pt)){
		std::cout << std::endl << "Recovery 128:   FAIL" << std::endl;
		return EXIT_FAILURE;
	}

	std::cout << std::endl << "Recovery 128:   OK" << std::endl;

	/* TWOFISH 192 */
	CryptoPP::ECB_Mode<CryptoPP::Twofish>::Encryption ecb_enc_twofish192(key192, sizeof key192);
	ecb_enc_twofish192.ProcessData(ct, pt, sizeof pt);

	std::cout << "Key 192:        ";
	print_raw_buffer(key192, sizeof key192);
	std::cout << std::endl << "Ciphertext 192: ";
	print_raw_buffer(ct, sizeof ct);

	CryptoPP::ECB_Mode<CryptoPP::Twofish>::Decryption ecb_dec_twofish192(key192, sizeof key192);
	ecb_dec_twofish192.ProcessData(vt, ct, sizeof ct);

	if (memcmp(pt, vt, sizeof pt)){
		std::cout << std::endl << "Recovery 192:   FAIL" << std::endl;
		return EXIT_FAILURE;
	}

	std::cout << std::endl << "Recovery 192:   OK" << std::endl;

	/* TWOFISH 256 */
	CryptoPP::ECB_Mode<CryptoPP::Twofish>::Encryption ecb_enc_twofish256(key256, sizeof key256);
	ecb_enc_twofish256.ProcessData(ct, pt, sizeof pt);

	std::cout << "Key 256:        ";
	print_raw_buffer(key256, sizeof key256);
	std::cout << std::endl << "Ciphertext 256: ";
	print_raw_buffer(ct, sizeof ct);

	CryptoPP::ECB_Mode<CryptoPP::Twofish>::Decryption ecb_dec_twofish256(key256, sizeof key256);
	ecb_dec_twofish256.ProcessData(vt, ct, sizeof ct);

	if (memcmp(pt, vt, sizeof pt)){
		std::cout << std::endl << "Recovery 256:   FAIL" << std::endl;
		return EXIT_FAILURE;
	}

	std::cout << std::endl << "Recovery 256:   OK" << std::endl;

	return EXIT_SUCCESS;
}
