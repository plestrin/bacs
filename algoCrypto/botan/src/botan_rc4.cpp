#include <iostream>
#include <iomanip>
#include <stdint.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include <unistd.h>

#include <botan/init.h>
#include <botan/arc4.h>

static void print_raw_buffer(Botan::byte* buffer, int buffer_length);

static void botan_patch();
static void botan_rc4();

int main() {
	botan_patch();
	botan_rc4();
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

static void botan_rc4() {
	unsigned char 	plaintext[] = "Hello World!";
	unsigned char 	key[] = "Key";
	unsigned char* 	ciphertext;
	unsigned char* 	deciphertext;
	Botan::ARC4 	rc4_enc;
	Botan::ARC4 	rc4_dec;
	unsigned int 	plaintext_length = sizeof(plaintext) - 1;

	ciphertext = (unsigned char*)malloc(plaintext_length);
	deciphertext = (unsigned char*)malloc(plaintext_length);

	if (ciphertext != NULL && deciphertext != NULL){
		std::cout << "Plaintext:  \"" << plaintext << "\"" << std::endl;
		std::cout << "Key:        \"" << key << "\"" << std::endl;

		rc4_enc.set_key((Botan::byte*)key, sizeof(key) - 1);
		rc4_enc.cipher((Botan::byte*)plaintext, (Botan::byte*)ciphertext, plaintext_length);

		rc4_dec.set_key((Botan::byte*)key, sizeof(key) - 1);
		rc4_dec.cipher((Botan::byte*)ciphertext, (Botan::byte*)deciphertext, plaintext_length);

		if (!memcmp(plaintext, deciphertext, plaintext_length)){
			std::cout << "Ciphertext: ";
			print_raw_buffer((Botan::byte*)ciphertext, plaintext_length);
			std::cout << std::endl << "Check:      OK" << std::endl;
		}
		else{
			std::cout << "Ciphertext: ";
			print_raw_buffer((Botan::byte*)ciphertext, plaintext_length);
			std::cout << std::endl << "Check:      FAIL" << std::endl;
		}
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