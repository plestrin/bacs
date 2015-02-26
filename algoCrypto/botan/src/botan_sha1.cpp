#include <iostream>
#include <iomanip>
#include <stdint.h>

#include <botan/init.h>
#include <botan/sha160.h>

static void print_raw_buffer(Botan::byte* buffer, int buffer_length);

int main() {
	char 			message[] = "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu";
	Botan::byte 	hash[20];
	Botan::SHA_160 	sha1;

	std::cout << "Plaintext: \"" << message << "\"" << std::endl;

	sha1.update((Botan::byte*)message, strlen(message));
	sha1.final(hash);

	std::cout << "SHA1 hash: ";
	print_raw_buffer(hash, 20);
	std::cout << std::endl;

	return 0;
}

static void print_raw_buffer(Botan::byte* buffer, int buffer_length){
	for (int i = 0; i < buffer_length; i++){
		if (buffer[i] & 0xf0){
			std::cout << std::hex << (buffer[i] & 0xff);
		}
		else{
			std::cout << "0" << std::hex << (buffer[i] & 0xff);
		}
	}
}