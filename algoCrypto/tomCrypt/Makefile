MISC_DIRECTORY 					:= /home/pierre/Documents/bacs/misc/

CC								:= gcc
CFLAGS							:= -W -Wall -pedantic -Wextra -I$(MISC_DIRECTORY) -g
LDFLAGS							:= -ltomcrypt

TEA_TOMCRYPT_EXAMPLE_NAME 		:= tom_tea
MD5_TOMCRYPT_EXAMPLE_NAME 		:= tom_md5
RC4_TOMCRYPT_EXAMPLE_NAME 		:= tom_rc4
AES_TOMCRYPT_EXAMPLE_NAME 		:= tom_aes
SHA1_TOMCRYPT_EXAMPLE_NAME 		:= tom_sha1
SHA256_TOMCRYPT_EXAMPLE_NAME 	:= tom_sha256
SHA512_TOMCRYPT_EXAMPLE_NAME 	:= tom_sha512
DES_TOMCRYPT_EXAMPLE_NAME 		:= tom_des
TWOFISH_TOMCRYPT_EXAMPLE_NAME 	:= tom_twofish

all: build/$(TEA_TOMCRYPT_EXAMPLE_NAME) 	\
	 build/$(MD5_TOMCRYPT_EXAMPLE_NAME) 	\
	 build/$(RC4_TOMCRYPT_EXAMPLE_NAME) 	\
	 build/$(AES_TOMCRYPT_EXAMPLE_NAME) 	\
	 build/$(SHA1_TOMCRYPT_EXAMPLE_NAME) 	\
	 build/$(SHA256_TOMCRYPT_EXAMPLE_NAME) 	\
	 build/$(SHA512_TOMCRYPT_EXAMPLE_NAME) 	\
	 build/$(DES_TOMCRYPT_EXAMPLE_NAME) 	\
	 build/$(TWOFISH_TOMCRYPT_EXAMPLE_NAME)

build/$(TEA_TOMCRYPT_EXAMPLE_NAME): build/tom_tea.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

build/$(MD5_TOMCRYPT_EXAMPLE_NAME): build/tom_md5.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

build/$(RC4_TOMCRYPT_EXAMPLE_NAME): build/tom_rc4.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

build/$(AES_TOMCRYPT_EXAMPLE_NAME): build/tom_aes.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

build/$(SHA1_TOMCRYPT_EXAMPLE_NAME): build/tom_sha1.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

build/$(SHA256_TOMCRYPT_EXAMPLE_NAME): build/tom_sha256.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

build/$(SHA512_TOMCRYPT_EXAMPLE_NAME): build/tom_sha512.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

build/$(DES_TOMCRYPT_EXAMPLE_NAME): build/tom_des.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

build/$(TWOFISH_TOMCRYPT_EXAMPLE_NAME): build/tom_twofish.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

build/%.o: src/%.c
	$(CC) -o $@ -c $< $(CFLAGS)

build/%.o: $(MISC_DIRECTORY)%.c
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	@ rm -f build/*

.PHONY: all clean


