MISC_DIRECTORY 				:= /home/pierre/Documents/bacs/misc/

CC							:= gcc
CFLAGS						:= -W -Wall -pedantic -Wextra -I$(MISC_DIRECTORY) -g
LDFLAGS						:= -ltomcrypt

MD5_TOMCRYPT_EXAMPLE_NAME 	:= MD5
RC4_TOMCRYPT_EXAMPLE_NAME 	:= RC4
AES_TOMCRYPT_EXAMPLE_NAME 	:= AES

all: build/$(MD5_TOMCRYPT_EXAMPLE_NAME) build/$(RC4_TOMCRYPT_EXAMPLE_NAME) build/$(AES_TOMCRYPT_EXAMPLE_NAME)

build/$(MD5_TOMCRYPT_EXAMPLE_NAME): build/md5_tom.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

build/$(RC4_TOMCRYPT_EXAMPLE_NAME): build/rc4_tom.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

build/$(AES_TOMCRYPT_EXAMPLE_NAME): build/aes_tom.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

build/%.o: src/%.c
	$(CC) -o $@ -c $< $(CFLAGS)

build/%.o: $(MISC_DIRECTORY)%.c
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	@ rm -f build/*

.PHONY: all


