MISC_DIRECTORY 			:= /home/pierre/Documents/bacs/misc/

CC						:= gcc
CFLAGS 					:= -W -Wall -pedantic -Wextra -g
CFLAGS_O0				:= -W -Wall -pedantic -Wextra -g
CFLAGS_O1				:= -W -Wall -pedantic -Wextra -O1
CFLAGS_O2				:= -W -Wall -pedantic -Wextra -O2
CFLAGS_O3				:= -W -Wall -pedantic -Wextra -O3
CFLAGS_Os				:= -W -Wall -pedantic -Wextra -Os
LDFLAGS					:=

TEA_TEST_NAME			:= test_tea
MD5_TEST_NAME 			:= test_md5
RC4_TEST_NAME 			:= test_rc4
AES_TEST_NAME 			:= test_aes
SHA1_TEST_NAME 			:= test_sha1

all: test_o0 test_o1 test_o2 test_o3 test_os

test_o0: build/$(TEA_TEST_NAME)_o0 build/$(MD5_TEST_NAME)_o0 build/$(RC4_TEST_NAME)_o0 build/$(AES_TEST_NAME)_o0 build/$(SHA1_TEST_NAME)_o0
test_o1: build/$(TEA_TEST_NAME)_o1 build/$(MD5_TEST_NAME)_o1 build/$(RC4_TEST_NAME)_o1 build/$(AES_TEST_NAME)_o1 build/$(SHA1_TEST_NAME)_o1
test_o2: build/$(TEA_TEST_NAME)_o2 build/$(MD5_TEST_NAME)_o2 build/$(RC4_TEST_NAME)_o2 build/$(AES_TEST_NAME)_o2 build/$(SHA1_TEST_NAME)_o2
test_o3: build/$(TEA_TEST_NAME)_o3 build/$(MD5_TEST_NAME)_o3 build/$(RC4_TEST_NAME)_o3 build/$(AES_TEST_NAME)_o3 build/$(SHA1_TEST_NAME)_o3
test_os: build/$(TEA_TEST_NAME)_os build/$(MD5_TEST_NAME)_os build/$(RC4_TEST_NAME)_os build/$(AES_TEST_NAME)_os build/$(SHA1_TEST_NAME)_os

build/$(TEA_TEST_NAME)_o0: build/TEA.o0 build/test_tea.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)
build/$(MD5_TEST_NAME)_o0: build/MD5.o0 build/test_md5.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)
build/$(RC4_TEST_NAME)_o0: build/RC4.o0 build/test_rc4.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)
build/$(AES_TEST_NAME)_o0: build/AES.o0 build/test_aes.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)
build/$(SHA1_TEST_NAME)_o0: build/SHA1.o0 build/test_sha1.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

build/$(TEA_TEST_NAME)_o1: build/TEA.o1 build/test_tea.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)
build/$(MD5_TEST_NAME)_o1: build/MD5.o1 build/test_md5.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)
build/$(RC4_TEST_NAME)_o1: build/RC4.o1 build/test_rc4.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)
build/$(AES_TEST_NAME)_o1: build/AES.o1 build/test_aes.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)
build/$(SHA1_TEST_NAME)_o1: build/SHA1.o1 build/test_sha1.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

build/$(TEA_TEST_NAME)_o2: build/TEA.o2 build/test_tea.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)
build/$(MD5_TEST_NAME)_o2: build/MD5.o2 build/test_md5.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)
build/$(RC4_TEST_NAME)_o2: build/RC4.o2 build/test_rc4.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)
build/$(AES_TEST_NAME)_o2: build/AES.o2 build/test_aes.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)
build/$(SHA1_TEST_NAME)_o2: build/SHA1.o2 build/test_sha1.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

build/$(TEA_TEST_NAME)_o3: build/TEA.o3 build/test_tea.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)
build/$(MD5_TEST_NAME)_o3: build/MD5.o3 build/test_md5.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)
build/$(RC4_TEST_NAME)_o3: build/RC4.o3 build/test_rc4.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)
build/$(AES_TEST_NAME)_o3: build/AES.o3 build/test_aes.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)
build/$(SHA1_TEST_NAME)_o3: build/SHA1.o3 build/test_sha1.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

build/$(TEA_TEST_NAME)_os: build/TEA.os build/test_tea.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)
build/$(MD5_TEST_NAME)_os: build/MD5.os build/test_md5.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)
build/$(RC4_TEST_NAME)_os: build/RC4.os build/test_rc4.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)
build/$(AES_TEST_NAME)_os: build/AES.os build/test_aes.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)
build/$(SHA1_TEST_NAME)_os: build/SHA1.os build/test_sha1.o build/printBuffer.o
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

.PHONY: all clean