#include <iostream>
#include <iomanip>
#include <stdint.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include <unistd.h>

#include <botan/init.h>
#include <botan/twofish.h>

static void print_raw_buffer(Botan::byte* buffer, int buffer_length);

static void botan_patch();
static void botan_twofish();

int main() {
	botan_patch();
	botan_twofish();
	return 0;
}

static void botan_patch(){
	void* 	lib_handle;
	char* 	func_addr;
	int32_t page_size;

	lib_handle = dlopen("/usr/lib/libbotan-1.10.so.0", RTLD_LAZY);
	if (lib_handle == NULL){
		std::cout << "ERROR: in " << __func__ << ", " << dlerror() << std::endl;
		return;
	}

	func_addr = (char*)dlsym(lib_handle, "_ZN5Botan26confirm_startup_self_testsERNS_17Algorithm_FactoryE");
	if (func_addr == NULL){
		std::cout << "ERROR: in " << __func__ << ", " << dlerror() << std::endl;
		goto exit;
	}

	page_size = getpagesize();

	if (mprotect(func_addr - ((unsigned long)func_addr % page_size), page_size, PROT_EXEC | PROT_READ | PROT_WRITE)){
		std::cout << "ERROR: in " << __func__ << ", unable to set memory write permission" << std::endl;
		goto exit;
	}

	*func_addr = 0xc3;

	if (mprotect(func_addr - ((long)func_addr % page_size), page_size, PROT_EXEC | PROT_READ)){
		std::cout << "ERROR: in " << __func__ << ", unable to reset memory permission" << std::endl;
		goto exit;
	}

	exit:
	if (dlclose(lib_handle)){
		std::cout << "ERROR: in " << __func__ << ", " << dlerror() << std::endl;
	}
}

static void botan_twofish() {
	unsigned char key_128[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
	unsigned char key_192[24] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17};
	unsigned char key_256[32] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};
	
	unsigned char plaintext[16] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
	unsigned char ciphertext[16];
	unsigned char deciphertext[16];

	Botan::Twofish 	twofish128;
	Botan::Twofish 	twofish192;
	Botan::Twofish 	twofish256;

	std::cout << "Plaintext:      ";
	print_raw_buffer((Botan::byte*)plaintext, 16);

	/* 128 bits TWOFISH */
	std::cout << std::endl << "Key 128:        ";
	print_raw_buffer((Botan::byte*)key_128, 16);

	twofish128.set_key((Botan::byte*)key_128, 16);
	twofish128.encrypt_n((Botan::byte*)plaintext, (Botan::byte*)ciphertext, 1);
	twofish128.decrypt_n((Botan::byte*)ciphertext, (Botan::byte*)deciphertext, 1);

	if (!memcmp(plaintext, deciphertext, 16)){
		std::cout << std::endl << "Ciphertext 128: ";
		print_raw_buffer((Botan::byte*)ciphertext, 16);
		std::cout << std::endl << "Recovery 128:   OK" << std::endl;
	}
	else{
		std::cout << std::endl << "Ciphertext 128: ";
		print_raw_buffer((Botan::byte*)ciphertext, 16);
		std::cout << std::endl << "Recovery 128:   FAIL" << std::endl;
	}

	/* 192 bits TWOFISH */
	std::cout << "Key 192:        ";
	print_raw_buffer((Botan::byte*)key_192, 24);

	twofish192.set_key((Botan::byte*)key_192, 24);
	twofish192.encrypt_n((Botan::byte*)plaintext, (Botan::byte*)ciphertext, 1);
	twofish192.decrypt_n((Botan::byte*)ciphertext, (Botan::byte*)deciphertext, 1);

	if (!memcmp(plaintext, deciphertext, 16)){
		std::cout << std::endl << "Ciphertext 192: ";
		print_raw_buffer((Botan::byte*)ciphertext, 16);
		std::cout << std::endl << "Recovery 192:   OK" << std::endl;
	}
	else{
		std::cout << std::endl << "Ciphertext 192: ";
		print_raw_buffer((Botan::byte*)ciphertext, 16);
		std::cout << std::endl << "Recovery 192:   FAIL" << std::endl;
	}

	/* 256 bits TWOFISH */
	std::cout << "Key 256:        ";
	print_raw_buffer((Botan::byte*)key_256, 32);

	twofish256.set_key((Botan::byte*)key_256, 32);
	twofish256.encrypt_n((Botan::byte*)plaintext, (Botan::byte*)ciphertext, 1);
	twofish256.decrypt_n((Botan::byte*)ciphertext, (Botan::byte*)deciphertext, 1);

	if (!memcmp(plaintext, deciphertext, 16)){
		std::cout << std::endl << "Ciphertext 256: ";
		print_raw_buffer((Botan::byte*)ciphertext, 16);
		std::cout << std::endl << "Recovery 256:   OK" << std::endl;
	}
	else{
		std::cout << std::endl << "Ciphertext 256: ";
		print_raw_buffer((Botan::byte*)ciphertext, 16);
		std::cout << std::endl << "Recovery 256:   FAIL" << std::endl;
	}
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