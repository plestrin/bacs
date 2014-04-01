MISC_DIRECTORY 				:= /home/pierre/Documents/bacs/misc/

CC							:= gcc
CFLAGS						:= -W -Wall -pedantic -Wextra -I$(MISC_DIRECTORY) -g
LDFLAGS						:= -lcrypto

AES_OPENSSL_EXAMPLE_NAME 	:= openssl_aes

all: build/$(AES_OPENSSL_EXAMPLE_NAME)

build/$(AES_OPENSSL_EXAMPLE_NAME): build/openssl_aes.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

build/%.o: src/%.c
	$(CC) -o $@ -c $< $(CFLAGS)

build/%.o: $(MISC_DIRECTORY)%.c
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	@ rm -f build/*

.PHONY: all clean
