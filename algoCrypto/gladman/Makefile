MISC_DIRECTORY 				:= /home/pierre/Documents/bacs/misc/
CC							:= gcc
CFLAGS 						:= -W -Wall -pedantic -Wextra -g -std=c99
LDFLAGS						:=

AES_NAME 					:= glad_aes

all: build/$(AES_NAME)

build/$(AES_NAME): build/glad_aes.o build/printBuffer.o build/aeskey.o build/aestab.o build/aescrypt.o build/aes_modes.o
	$(CC) -o $@ $^ $(LDFLAGS)

build/%.o: src/%.c
	$(CC) -o $@ -c $< $(CFLAGS) -I$(MISC_DIRECTORY) -I../ -I./lib/

build/%.o: $(MISC_DIRECTORY)/%.c
	$(CC) -o $@ -c $< $(CFLAGS)

build/%.o: lib/%.c
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	@ rm -f build/*

.PHONY: all clean