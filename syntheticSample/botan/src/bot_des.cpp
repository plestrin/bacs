#include <iostream>
#include <iomanip>
#include <stdint.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include <unistd.h>

#include <botan/init.h>
#include <botan/des.h>

static void print_raw_buffer(Botan::byte* buffer, int buffer_length);

static void botan_patch();
static void botan_des();

int main() {
	botan_patch();
	botan_des();
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

static void botan_des() {
	unsigned char key[8] = {0x75, 0x29, 0x79, 0x38, 0x75, 0x92, 0xcb, 0x70};
	
	unsigned char plaintext[8] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
	unsigned char ciphertext[8];
	unsigned char deciphertext[8];

	Botan::DES 	des;

	std::cout << "Plaintext:  ";
	print_raw_buffer((Botan::byte*)plaintext, 8);

	std::cout << std::endl << "Key:        ";
	print_raw_buffer((Botan::byte*)key, 8);

	des.set_key((Botan::byte*)key, 8);
	des.encrypt_n((Botan::byte*)plaintext, (Botan::byte*)ciphertext, 1);
	des.decrypt_n((Botan::byte*)ciphertext, (Botan::byte*)deciphertext, 1);

	if (!memcmp(plaintext, deciphertext, 8)){
		std::cout << std::endl << "Ciphertext: ";
		print_raw_buffer((Botan::byte*)ciphertext, 8);
		std::cout << std::endl << "Recovery:   OK" << std::endl;
	}
	else{
		std::cout << std::endl << "Ciphertext: ";
		print_raw_buffer((Botan::byte*)ciphertext, 8);
		std::cout << std::endl << "Recovery:   FAIL" << std::endl;
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