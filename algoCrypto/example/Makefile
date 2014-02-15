MISC_DIRECTORY 				:= /home/pierre/Documents/bacs/misc/

CC							:= gcc
CFLAGS 						:= -W -Wall -pedantic -Wextra -g
CFLAGS_O0					:= -W -Wall -pedantic -Wextra -g
CFLAGS_O1					:= -W -Wall -pedantic -Wextra -O1
CFLAGS_O2					:= -W -Wall -pedantic -Wextra -O2
CFLAGS_O3					:= -W -Wall -pedantic -Wextra -O3
CFLAGS_Os					:= -W -Wall -pedantic -Wextra -Os
LDFLAGS						:=

TEA_EXAMPLE_NAME			:= TEA
MD5_EXAMPLE_NAME 			:= MD5
RC4_EXAMPLE_NAME 			:= RC4
AES_EXAMPLE_NAME 			:= AES
SHA1_EXAMPLE_NAME 			:= SHA1

all: example_o0 example_o1 example_o2 example_o3 example_os

example_o0: build/$(TEA_EXAMPLE_NAME)_o0 build/$(MD5_EXAMPLE_NAME)_o0 build/$(RC4_EXAMPLE_NAME)_o0 build/$(AES_EXAMPLE_NAME)_o0 build/$(SHA1_EXAMPLE_NAME)_o0
example_o1: build/$(TEA_EXAMPLE_NAME)_o1 build/$(MD5_EXAMPLE_NAME)_o1 build/$(RC4_EXAMPLE_NAME)_o1 build/$(AES_EXAMPLE_NAME)_o1 build/$(SHA1_EXAMPLE_NAME)_o1
example_o2: build/$(TEA_EXAMPLE_NAME)_o2 build/$(MD5_EXAMPLE_NAME)_o2 build/$(RC4_EXAMPLE_NAME)_o2 build/$(AES_EXAMPLE_NAME)_o2 build/$(SHA1_EXAMPLE_NAME)_o2
example_o3: build/$(TEA_EXAMPLE_NAME)_o3 build/$(MD5_EXAMPLE_NAME)_o3 build/$(RC4_EXAMPLE_NAME)_o3 build/$(AES_EXAMPLE_NAME)_o3 build/$(SHA1_EXAMPLE_NAME)_o3
example_os: build/$(TEA_EXAMPLE_NAME)_os build/$(MD5_EXAMPLE_NAME)_os build/$(RC4_EXAMPLE_NAME)_os build/$(AES_EXAMPLE_NAME)_os build/$(SHA1_EXAMPLE_NAME)_os

build/$(TEA_EXAMPLE_NAME)_o0: build/TEA.o0 build/tea_test.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)
build/$(MD5_EXAMPLE_NAME)_o0: build/MD5.o0 build/md5_test.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)
build/$(RC4_EXAMPLE_NAME)_o0: build/RC4.o0 build/rc4_test.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)
build/$(AES_EXAMPLE_NAME)_o0: build/AES.o0 build/aes_test.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)
build/$(SHA1_EXAMPLE_NAME)_o0: build/SHA1.o0 build/sha1_test.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

build/$(TEA_EXAMPLE_NAME)_o1: build/TEA.o1 build/tea_test.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)
build/$(MD5_EXAMPLE_NAME)_o1: build/MD5.o1 build/md5_test.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)
build/$(RC4_EXAMPLE_NAME)_o1: build/RC4.o1 build/rc4_test.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)
build/$(AES_EXAMPLE_NAME)_o1: build/AES.o1 build/aes_test.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)
build/$(SHA1_EXAMPLE_NAME)_o1: build/SHA1.o1 build/sha1_test.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

build/$(TEA_EXAMPLE_NAME)_o2: build/TEA.o2 build/tea_test.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)
build/$(MD5_EXAMPLE_NAME)_o2: build/MD5.o2 build/md5_test.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)
build/$(RC4_EXAMPLE_NAME)_o2: build/RC4.o2 build/rc4_test.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)
build/$(AES_EXAMPLE_NAME)_o2: build/AES.o2 build/aes_test.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)
build/$(SHA1_EXAMPLE_NAME)_o2: build/SHA1.o2 build/sha1_test.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

build/$(TEA_EXAMPLE_NAME)_o3: build/TEA.o3 build/tea_test.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)
build/$(MD5_EXAMPLE_NAME)_o3: build/MD5.o3 build/md5_test.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)
build/$(RC4_EXAMPLE_NAME)_o3: build/RC4.o3 build/rc4_test.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)
build/$(AES_EXAMPLE_NAME)_o3: build/AES.o3 build/aes_test.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)
build/$(SHA1_EXAMPLE_NAME)_o3: build/SHA1.o3 build/sha1_test.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

build/$(TEA_EXAMPLE_NAME)_os: build/TEA.os build/tea_test.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)
build/$(MD5_EXAMPLE_NAME)_os: build/MD5.os build/md5_test.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)
build/$(RC4_EXAMPLE_NAME)_os: build/RC4.os build/rc4_test.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)
build/$(AES_EXAMPLE_NAME)_os: build/AES.os build/aes_test.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)
build/$(SHA1_EXAMPLE_NAME)_os: build/SHA1.os build/sha1_test.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

build/%.o: src/%.c
	$(CC) -o $@ -c $< $(CFLAGS) -I$(MISC_DIRECTORY) -I../

build/%.o: $(MISC_DIRECTORY)/%.c
	$(CC) -o $@ -c $< $(CFLAGS)

build/%.o0: ../%.c
	$(CC) -o $@ -c $< $(CFLAGS_O0)

build/%.o1: ../%.c
	$(CC) -o $@ -c $< $(CFLAGS_O1)

build/%.o2: ../%.c
	$(CC) -o $@ -c $< $(CFLAGS_O2)

build/%.o3: ../%.c
	$(CC) -o $@ -c $< $(CFLAGS_O3)

build/%.os: ../%.c
	$(CC) -o $@ -c $< $(CFLAGS_Os)

clean:
	@ rm -f build/*

.PHONY: all