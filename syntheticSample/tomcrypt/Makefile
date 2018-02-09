MISC_DIRECTORY 					:= ../../misc/

CC								:= gcc
CFLAGS							:= -W -Wall -pedantic -Wextra -I$(MISC_DIRECTORY) -O2 -std=gnu99
LDFLAGS							:= -ltomcrypt

TEA_TOMCRYPT_EXAMPLE_NAME 		:= tom_tea
ECB_TEA_TOMCRYPT_EXAMPLE_NAME 	:= tom_ecb_tea
CBC_TEA_TOMCRYPT_EXAMPLE_NAME 	:= tom_cbc_tea
OFB_TEA_TOMCRYPT_EXAMPLE_NAME 	:= tom_ofb_tea
CFB_TEA_TOMCRYPT_EXAMPLE_NAME 	:= tom_cfb_tea
CTR_TEA_TOMCRYPT_EXAMPLE_NAME 	:= tom_ctr_tea
MD5_TOMCRYPT_EXAMPLE_NAME 		:= tom_md5
HMAC_MD5_TOMCRYPT_EXAMPLE_NAME 	:= tom_hmac_md5
RC4_TOMCRYPT_EXAMPLE_NAME 		:= tom_rc4
AES_TOMCRYPT_EXAMPLE_NAME 		:= tom_aes
ECB_AES_TOMCRYPT_EXAMPLE_NAME 	:= tom_ecb_aes
CBC_AES_TOMCRYPT_EXAMPLE_NAME 	:= tom_cbc_aes
OFB_AES_TOMCRYPT_EXAMPLE_NAME 	:= tom_ofb_aes
CFB_AES_TOMCRYPT_EXAMPLE_NAME 	:= tom_cfb_aes
CTR_AES_TOMCRYPT_EXAMPLE_NAME 	:= tom_ctr_aes
OCB_AES_TOMCRYPT_EXAMPLE_NAME 	:= tom_ocb_aes
SHA1_TOMCRYPT_EXAMPLE_NAME 		:= tom_sha1
HMAC_SHA1_TOMCRYPT_EXAMPLE_NAME := tom_hmac_sha1
SHA256_TOMCRYPT_EXAMPLE_NAME 	:= tom_sha256
SHA512_TOMCRYPT_EXAMPLE_NAME 	:= tom_sha512
DES_TOMCRYPT_EXAMPLE_NAME 		:= tom_des
TWOFISH_TOMCRYPT_EXAMPLE_NAME 	:= tom_twofish

all: build build/$(TEA_TOMCRYPT_EXAMPLE_NAME) 	\
	 build/$(ECB_TEA_TOMCRYPT_EXAMPLE_NAME) 	\
	 build/$(CBC_TEA_TOMCRYPT_EXAMPLE_NAME) 	\
	 build/$(OFB_TEA_TOMCRYPT_EXAMPLE_NAME) 	\
	 build/$(CFB_TEA_TOMCRYPT_EXAMPLE_NAME) 	\
	 build/$(CTR_TEA_TOMCRYPT_EXAMPLE_NAME) 	\
	 build/$(MD5_TOMCRYPT_EXAMPLE_NAME) 		\
	 build/$(HMAC_MD5_TOMCRYPT_EXAMPLE_NAME) 	\
	 build/$(RC4_TOMCRYPT_EXAMPLE_NAME) 		\
	 build/$(AES_TOMCRYPT_EXAMPLE_NAME) 		\
	 build/$(ECB_AES_TOMCRYPT_EXAMPLE_NAME) 	\
	 build/$(CBC_AES_TOMCRYPT_EXAMPLE_NAME) 	\
	 build/$(OFB_AES_TOMCRYPT_EXAMPLE_NAME) 	\
	 build/$(CFB_AES_TOMCRYPT_EXAMPLE_NAME) 	\
	 build/$(CTR_AES_TOMCRYPT_EXAMPLE_NAME) 	\
	 build/$(OCB_AES_TOMCRYPT_EXAMPLE_NAME) 	\
	 build/$(SHA1_TOMCRYPT_EXAMPLE_NAME) 		\
	 build/$(HMAC_SHA1_TOMCRYPT_EXAMPLE_NAME) 	\
	 build/$(SHA256_TOMCRYPT_EXAMPLE_NAME) 		\
	 build/$(SHA512_TOMCRYPT_EXAMPLE_NAME) 		\
	 build/$(DES_TOMCRYPT_EXAMPLE_NAME) 		\
	 build/$(TWOFISH_TOMCRYPT_EXAMPLE_NAME)

build:
	mkdir -p build

build/$(TEA_TOMCRYPT_EXAMPLE_NAME): build/tom_tea.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

build/$(ECB_TEA_TOMCRYPT_EXAMPLE_NAME): build/tom_ecb_tea.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

build/$(CBC_TEA_TOMCRYPT_EXAMPLE_NAME): build/tom_cbc_tea.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

build/$(OFB_TEA_TOMCRYPT_EXAMPLE_NAME): build/tom_ofb_tea.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

build/$(CFB_TEA_TOMCRYPT_EXAMPLE_NAME): build/tom_cfb_tea.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

build/$(CTR_TEA_TOMCRYPT_EXAMPLE_NAME): build/tom_ctr_tea.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

build/$(MD5_TOMCRYPT_EXAMPLE_NAME): build/tom_md5.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

build/$(HMAC_MD5_TOMCRYPT_EXAMPLE_NAME): build/tom_hmac_md5.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

build/$(RC4_TOMCRYPT_EXAMPLE_NAME): build/tom_rc4.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

build/$(AES_TOMCRYPT_EXAMPLE_NAME): build/tom_aes.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

build/$(ECB_AES_TOMCRYPT_EXAMPLE_NAME): build/tom_ecb_aes.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

build/$(CBC_AES_TOMCRYPT_EXAMPLE_NAME): build/tom_cbc_aes.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

build/$(OFB_AES_TOMCRYPT_EXAMPLE_NAME): build/tom_ofb_aes.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

build/$(CFB_AES_TOMCRYPT_EXAMPLE_NAME): build/tom_cfb_aes.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

build/$(CTR_AES_TOMCRYPT_EXAMPLE_NAME): build/tom_ctr_aes.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

build/$(OCB_AES_TOMCRYPT_EXAMPLE_NAME): build/tom_ocb_aes.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

build/$(SHA1_TOMCRYPT_EXAMPLE_NAME): build/tom_sha1.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

build/$(HMAC_SHA1_TOMCRYPT_EXAMPLE_NAME): build/tom_hmac_sha1.o build/printBuffer.o
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
