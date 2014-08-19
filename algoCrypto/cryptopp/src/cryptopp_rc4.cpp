#include <iostream>
#include <iomanip>

#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1

#include <cryptopp/arc4.h>

void print_raw_buffer(byte* buffer, int buffer_length);

int main(int argc, char* argv[]) {
	byte 					plaintext[] = "Hello World!";
	byte					key[] = "Key";
	byte* 					ciphertext;
	byte* 					deciphertext;
	CryptoPP::Weak::ARC4* 	rc4_enc;
	CryptoPP::Weak::ARC4* 	rc4_dec;

	ciphertext = (byte*)malloc(strlen((char*)plaintext));
	deciphertext = (byte*)malloc(strlen((char*)plaintext));

	if (ciphertext != NULL && deciphertext != NULL){
		std::cout << "Plaintext:  \"" << plaintext << "\"" << std::endl;
		std::cout << "Key:        \"" << key << "\"" << std::endl;

		rc4_enc = new CryptoPP::Weak::ARC4(key, strlen((char*)key));
		rc4_enc->ProcessData(ciphertext, plaintext, strlen((char*)plaintext));

		std::cout << "Ciphertext: ";
		print_raw_buffer(ciphertext, strlen((char*)plaintext));
		std::cout << std::endl;

		rc4_dec = new CryptoPP::Weak::ARC4(key, strlen((char*)key));
		rc4_dec->ProcessData(deciphertext, ciphertext, strlen((char*)plaintext));

		if (memcmp(plaintext, deciphertext, strlen((char*)plaintext)) == 0){
			std::cout << "Check:      OK" << std::endl;
		}
		else{
			std::cout << "Check:      FAIL" << std::endl;
		}

		free(ciphertext);
		free(deciphertext);
	}
	else{
		std:: cout << "ERROR: in " <<__func__ << ", unable to allocate memory" << std::endl;
	}

	/*md5.CalculateDigest(hash, (unsigned char*)message, strlen(message));

	std::cout << "Plaintext: " << message << std::endl;
	std::cout << "MD5 hash:  ";
	print_raw_buffer(hash, sizeof(hash));
	std::cout << std::endl;*/

	return 0;
}

void print_raw_buffer(byte* buffer, int buffer_length){
	for (int i = 0; i < buffer_length; i++){
		if (buffer[i] & 0xf0){
			std::cout << std::hex << (buffer[i] & 0xff);
		}
		else{
			std::cout << "0" << std::hex << (buffer[i] & 0xff);
		}
	}
}