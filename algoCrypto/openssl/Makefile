MISC_DIRECTORY 				:= /home/pierre/Documents/bacs/misc/

CC							:= gcc
CFLAGS						:= -W -Wall -pedantic -Wextra -I$(MISC_DIRECTORY) -O2
LDFLAGS						:= -lcrypto

AES_OPENSSL_EXAMPLE_NAME 	:= openssl_aes
MD5_OPENSSL_EXAMPLE_NAME 	:= openssl_md5
RC4_OPENSSL_EXAMPLE_NAME 	:= openssl_rc4
DES_OPENSSL_EXAMPLE_NAME 	:= openssl_des

all: build/$(AES_OPENSSL_EXAMPLE_NAME) build/$(MD5_OPENSSL_EXAMPLE_NAME) build/$(RC4_OPENSSL_EXAMPLE_NAME) build/$(DES_OPENSSL_EXAMPLE_NAME)

build/$(AES_OPENSSL_EXAMPLE_NAME): build/openssl_aes.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

build/$(MD5_OPENSSL_EXAMPLE_NAME): build/openssl_md5.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

build/$(RC4_OPENSSL_EXAMPLE_NAME): build/openssl_rc4.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

build/$(DES_OPENSSL_EXAMPLE_NAME): build/openssl_des.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

build/%.o: src/%.c
	$(CC) -o $@ -c $< $(CFLAGS)

build/%.o: $(MISC_DIRECTORY)%.c
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	@ rm -f build/*

.PHONY: all clean
