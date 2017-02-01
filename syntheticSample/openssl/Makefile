MISC_DIRECTORY 	:= ../../misc/

CC				:= gcc
CFLAGS			:= -W -Wall -pedantic -Wextra -I$(MISC_DIRECTORY) -O2 -std=gnu99
LDFLAGS			:= -lcrypto

AES_OPENSSL_EXAMPLE_NAME 		:= open_aes
ECB_AES_OPENSSL_EXAMPLE_NAME 	:= open_ecb_aes
CBC_AES_OPENSSL_EXAMPLE_NAME 	:= open_cbc_aes
OFB_AES_OPENSSL_EXAMPLE_NAME 	:= open_ofb_aes
CFB_AES_OPENSSL_EXAMPLE_NAME 	:= open_cfb_aes
CTR_AES_OPENSSL_EXAMPLE_NAME 	:= open_ctr_aes
MD5_OPENSSL_EXAMPLE_NAME 		:= open_md5
RC4_OPENSSL_EXAMPLE_NAME 		:= open_rc4
DES_OPENSSL_EXAMPLE_NAME 		:= open_des
SHA1_OPENSSL_EXAMPLE_NAME 		:= open_sha1
HMAC_MD5_OPENSSL_EXAMPLE_NAME 	:= open_hmac_md5

all: 	build build/$(AES_OPENSSL_EXAMPLE_NAME) \
		build/$(ECB_AES_OPENSSL_EXAMPLE_NAME) 	\
		build/$(CBC_AES_OPENSSL_EXAMPLE_NAME) 	\
		build/$(OFB_AES_OPENSSL_EXAMPLE_NAME) 	\
		build/$(CFB_AES_OPENSSL_EXAMPLE_NAME) 	\
		build/$(CTR_AES_OPENSSL_EXAMPLE_NAME) 	\
		build/$(MD5_OPENSSL_EXAMPLE_NAME) 		\
		build/$(RC4_OPENSSL_EXAMPLE_NAME) 		\
		build/$(DES_OPENSSL_EXAMPLE_NAME) 		\
		build/$(SHA1_OPENSSL_EXAMPLE_NAME) 		\
		build/$(HMAC_MD5_OPENSSL_EXAMPLE_NAME)

build:
	mkdir -p build

build/$(AES_OPENSSL_EXAMPLE_NAME): build/open_aes.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

build/$(ECB_AES_OPENSSL_EXAMPLE_NAME): build/open_ecb_aes.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

build/$(CBC_AES_OPENSSL_EXAMPLE_NAME): build/open_cbc_aes.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

build/$(OFB_AES_OPENSSL_EXAMPLE_NAME): build/open_ofb_aes.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

build/$(CFB_AES_OPENSSL_EXAMPLE_NAME): build/open_cfb_aes.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

build/$(CTR_AES_OPENSSL_EXAMPLE_NAME): build/open_ctr_aes.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

build/$(MD5_OPENSSL_EXAMPLE_NAME): build/open_md5.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

build/$(RC4_OPENSSL_EXAMPLE_NAME): build/open_rc4.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

build/$(DES_OPENSSL_EXAMPLE_NAME): build/open_des.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

build/$(SHA1_OPENSSL_EXAMPLE_NAME): build/open_sha1.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

build/$(HMAC_MD5_OPENSSL_EXAMPLE_NAME): build/open_hmac_md5.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

build/%.o: src/%.c
	$(CC) -o $@ -c $< $(CFLAGS)

build/%.o: $(MISC_DIRECTORY)%.c
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	@ rm -f build/*

.PHONY: all clean
