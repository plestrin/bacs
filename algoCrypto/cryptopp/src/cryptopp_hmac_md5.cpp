#include <iostream>
#include <iomanip>

#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1

#include <cryptopp/md5.h>
#include <cryptopp/hmac.h>

void print_raw_buffer(byte* buffer, int buffer_length);

int main(){
	char message[] = "Hello I am a test vector for the HMAC MD5. Since I am 92 bytes wide I lay on several blocks.";
	char key[] = "1 4m 4 53cr3t k3y";
	byte hash[CryptoPP::Weak::MD5::DIGESTSIZE];
	CryptoPP::HMAC<CryptoPP::Weak::MD5> hmac;

	hmac.SetKey((unsigned char*)key, strlen(key));
	hmac.CalculateDigest(hash, (unsigned char*)message, sizeof(message) - 1);

	std::cout << "Plaintext: \"" << message << "\"" << std::endl;
	std::cout << "Key:       \"" << key << "\"" << std::endl;
	std::cout << "MD5 HMAC:  ";
	print_raw_buffer(hash, sizeof(hash));
	std::cout << std::endl;

	return EXIT_SUCCESS;
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