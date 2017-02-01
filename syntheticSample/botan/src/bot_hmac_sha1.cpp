#include <iostream>
#include <iomanip>
#include <stdint.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include <unistd.h>

#include <botan/init.h>
#include <botan/sha160.h>
#include <botan/hmac.h>

static void print_raw_buffer(Botan::byte* buffer, int buffer_length);

static void botan_patch();
static void botan_hmac_sha1();

int main() {
	botan_patch();
	botan_hmac_sha1();
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

static void botan_hmac_sha1() {
	char 			message[] = "Hello I am a test vector for the HMAC SHA1. Since I am 93 bytes wide I lay on several blocks.";
	char 			key[] = "1 4m 4 53cr3t k3y";
	Botan::byte 	hash_mac[20];
	Botan::HMAC* 	hmac;

	hmac = new Botan::HMAC(new Botan::SHA_160());

	std::cout << "Plaintext: \"" << message << "\"" << std::endl;
	std::cout << "Key:       \"" << key << "\"" << std::endl;

	hmac->set_key((Botan::byte*)key, strlen(key));
	hmac->update((Botan::byte*)message, strlen(message));
	hmac->final(hash_mac);

	std::cout << "SHA1 HMAC: ";
	print_raw_buffer(hash_mac, sizeof(hash_mac));
	std::cout << std::endl;

	delete(hmac);
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