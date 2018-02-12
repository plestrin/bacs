MISC_DIRECTORY 					:= ../../misc/

CC								:= gcc
CFLAGS							:= -W -Wall -pedantic -Wextra -I$(MISC_DIRECTORY) -O2 -std=gnu99
LDFLAGS							:= -lnettle

AES_NETTLE_EXAMPLE_NAME 		:= net_aes
CBC_AES_NETTLE_EXAMPLE_NAME 	:= net_cbc_aes
CTR_AES_NETTLE_EXAMPLE_NAME 	:= net_ctr_aes
MD5_NETTLE_EXAMPLE_NAME 		:= net_md5
HMAC_MD5_NETTLE_EXAMPLE_NAME 	:= net_hmac_md5
HMAC_SHA1_NETTLE_EXAMPLE_NAME 	:= net_hmac_sha1
SHA1_NETTLE_EXAMPLE_NAME 		:= net_sha1
RC4_NETTLE_EXAMPLE_NAME 		:= net_rc4

all: build build/$(AES_NETTLE_EXAMPLE_NAME) 	\
	 build/$(CBC_AES_NETTLE_EXAMPLE_NAME) 		\
	 build/$(CTR_AES_NETTLE_EXAMPLE_NAME) 		\
	 build/$(MD5_NETTLE_EXAMPLE_NAME) 			\
	 build/$(HMAC_MD5_NETTLE_EXAMPLE_NAME) 		\
	 build/$(HMAC_SHA1_NETTLE_EXAMPLE_NAME) 	\
	 build/$(SHA1_NETTLE_EXAMPLE_NAME) 			\
	 build/$(RC4_NETTLE_EXAMPLE_NAME)

build:
	mkdir -p build

build/$(AES_NETTLE_EXAMPLE_NAME): build/net_aes.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

build/$(CBC_AES_NETTLE_EXAMPLE_NAME): build/net_cbc_aes.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

build/$(CTR_AES_NETTLE_EXAMPLE_NAME): build/net_ctr_aes.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

build/$(MD5_NETTLE_EXAMPLE_NAME): build/net_md5.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

build/$(SHA1_NETTLE_EXAMPLE_NAME): build/net_sha1.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

build/$(HMAC_MD5_NETTLE_EXAMPLE_NAME): build/net_hmac_md5.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

build/$(HMAC_SHA1_NETTLE_EXAMPLE_NAME): build/net_hmac_sha1.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

build/$(RC4_NETTLE_EXAMPLE_NAME): build/net_rc4.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

build/%.o: src/%.c
	$(CC) -o $@ -c $< $(CFLAGS)

build/%.o: $(MISC_DIRECTORY)%.c
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	@ rm -f build/*

.PHONY: all clean
