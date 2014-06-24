MISC_DIRECTORY 			:= /home/pierre/Documents/bacs/misc/

GCC_CC					:= gcc
GCC_CFLAGS 				:= -W -Wall -pedantic -Wextra
CLANG_CC 				:= clang
CLANG_CFLAGS 			:= -pedantic -Weverything -Wno-cast-align -Wno-newline-eof -Wno-unused-macros

TEA_TEST_NAME			:= test_tea
MD5_TEST_NAME 			:= test_md5
RC4_TEST_NAME 			:= test_rc4
AES_TEST_NAME 			:= test_aes
SHA1_TEST_NAME 			:= test_sha1

all: test_go0 test_go1 test_go2 test_go3 test_gos test_co0 test_co1 test_co2 test_co3 test_coz

# GCC compiler
test_go0: build/$(TEA_TEST_NAME)_go0 build/$(MD5_TEST_NAME)_go0 build/$(RC4_TEST_NAME)_go0 build/$(AES_TEST_NAME)_go0 build/$(SHA1_TEST_NAME)_go0
test_go1: build/$(TEA_TEST_NAME)_go1 build/$(MD5_TEST_NAME)_go1 build/$(RC4_TEST_NAME)_go1 build/$(AES_TEST_NAME)_go1 build/$(SHA1_TEST_NAME)_go1
test_go2: build/$(TEA_TEST_NAME)_go2 build/$(MD5_TEST_NAME)_go2 build/$(RC4_TEST_NAME)_go2 build/$(AES_TEST_NAME)_go2 build/$(SHA1_TEST_NAME)_go2
test_go3: build/$(TEA_TEST_NAME)_go3 build/$(MD5_TEST_NAME)_go3 build/$(RC4_TEST_NAME)_go3 build/$(AES_TEST_NAME)_go3 build/$(SHA1_TEST_NAME)_go3
test_gos: build/$(TEA_TEST_NAME)_gos build/$(MD5_TEST_NAME)_gos build/$(RC4_TEST_NAME)_gos build/$(AES_TEST_NAME)_gos build/$(SHA1_TEST_NAME)_gos

build/$(TEA_TEST_NAME)_go0: build/TEA.go0 build/test_tea.go0 build/printBuffer.go0
	$(GCC_CC) -o $@ $^
build/$(MD5_TEST_NAME)_go0: build/MD5.go0 build/test_md5.go0 build/printBuffer.go0
	$(GCC_CC) -o $@ $^
build/$(RC4_TEST_NAME)_go0: build/RC4.go0 build/test_rc4.go0 build/printBuffer.go0
	$(GCC_CC) -o $@ $^
build/$(AES_TEST_NAME)_go0: build/AES.go0 build/test_aes.go0 build/printBuffer.go0
	$(GCC_CC) -o $@ $^
build/$(SHA1_TEST_NAME)_go0: build/SHA1.go0 build/test_sha1.go0 build/printBuffer.go0
	$(GCC_CC) -o $@ $^

build/$(TEA_TEST_NAME)_go1: build/TEA.go1 build/test_tea.go0 build/printBuffer.go0
	$(GCC_CC) -o $@ $^
build/$(MD5_TEST_NAME)_go1: build/MD5.go1 build/test_md5.go0 build/printBuffer.go0
	$(GCC_CC) -o $@ $^
build/$(RC4_TEST_NAME)_go1: build/RC4.go1 build/test_rc4.go0 build/printBuffer.go0
	$(GCC_CC) -o $@ $^
build/$(AES_TEST_NAME)_go1: build/AES.go1 build/test_aes.go0 build/printBuffer.go0
	$(GCC_CC) -o $@ $^
build/$(SHA1_TEST_NAME)_go1: build/SHA1.go1 build/test_sha1.go0 build/printBuffer.go0
	$(GCC_CC) -o $@ $^

build/$(TEA_TEST_NAME)_go2: build/TEA.go2 build/test_tea.go0 build/printBuffer.go0
	$(GCC_CC) -o $@ $^
build/$(MD5_TEST_NAME)_go2: build/MD5.go2 build/test_md5.go0 build/printBuffer.go0
	$(GCC_CC) -o $@ $^
build/$(RC4_TEST_NAME)_go2: build/RC4.go2 build/test_rc4.go0 build/printBuffer.go0
	$(GCC_CC) -o $@ $^
build/$(AES_TEST_NAME)_go2: build/AES.go2 build/test_aes.go0 build/printBuffer.go0
	$(GCC_CC) -o $@ $^
build/$(SHA1_TEST_NAME)_go2: build/SHA1.go2 build/test_sha1.go0 build/printBuffer.go0
	$(GCC_CC) -o $@ $^

build/$(TEA_TEST_NAME)_go3: build/TEA.go3 build/test_tea.go0 build/printBuffer.go0
	$(GCC_CC) -o $@ $^
build/$(MD5_TEST_NAME)_go3: build/MD5.go3 build/test_md5.go0 build/printBuffer.go0
	$(GCC_CC) -o $@ $^
build/$(RC4_TEST_NAME)_go3: build/RC4.go3 build/test_rc4.go0 build/printBuffer.go0
	$(GCC_CC) -o $@ $^
build/$(AES_TEST_NAME)_go3: build/AES.go3 build/test_aes.go0 build/printBuffer.go0
	$(GCC_CC) -o $@ $^
build/$(SHA1_TEST_NAME)_go3: build/SHA1.go3 build/test_sha1.go0 build/printBuffer.go0
	$(GCC_CC) -o $@ $^

build/$(TEA_TEST_NAME)_gos: build/TEA.gos build/test_tea.go0 build/printBuffer.go0
	$(GCC_CC) -o $@ $^
build/$(MD5_TEST_NAME)_gos: build/MD5.gos build/test_md5.go0 build/printBuffer.go0
	$(GCC_CC) -o $@ $^
build/$(RC4_TEST_NAME)_gos: build/RC4.gos build/test_rc4.go0 build/printBuffer.go0
	$(GCC_CC) -o $@ $^
build/$(AES_TEST_NAME)_gos: build/AES.gos build/test_aes.go0 build/printBuffer.go0
	$(GCC_CC) -o $@ $^
build/$(SHA1_TEST_NAME)_gos: build/SHA1.gos build/test_sha1.go0 build/printBuffer.go0
	$(GCC_CC) -o $@ $^

build/%.go0: src/%.c
	$(GCC_CC) -o $@ -c $< $(GCC_CFLAGS) -g -I$(MISC_DIRECTORY) -I../

build/%.go0: $(MISC_DIRECTORY)%.c
	$(GCC_CC) -o $@ -c $< $(GCC_CFLAGS) -g

build/%.go0: ../%.c
	$(GCC_CC) -o $@ -c $< $(GCC_CFLAGS) -O0

build/%.go1: ../%.c
	$(GCC_CC) -o $@ -c $< $(GCC_CFLAGS) -O1

build/%.go2: ../%.c
	$(GCC_CC) -o $@ -c $< $(GCC_CFLAGS) -O2

build/%.go3: ../%.c
	$(GCC_CC) -o $@ -c $< $(GCC_CFLAGS) -O3

build/%.gos: ../%.c
	$(GCC_CC) -o $@ -c $< $(GCC_CFLAGS) -Os

# CLANG compiler
test_co0: build/$(TEA_TEST_NAME)_co0 build/$(MD5_TEST_NAME)_co0 build/$(RC4_TEST_NAME)_co0 build/$(AES_TEST_NAME)_co0 build/$(SHA1_TEST_NAME)_co0
test_co1: build/$(TEA_TEST_NAME)_co1 build/$(MD5_TEST_NAME)_co1 build/$(RC4_TEST_NAME)_co1 build/$(AES_TEST_NAME)_co1 build/$(SHA1_TEST_NAME)_co1
test_co2: build/$(TEA_TEST_NAME)_co2 build/$(MD5_TEST_NAME)_co2 build/$(RC4_TEST_NAME)_co2 build/$(AES_TEST_NAME)_co2 build/$(SHA1_TEST_NAME)_co2
test_co3: build/$(TEA_TEST_NAME)_co3 build/$(MD5_TEST_NAME)_co3 build/$(RC4_TEST_NAME)_co3 build/$(AES_TEST_NAME)_co3 build/$(SHA1_TEST_NAME)_co3
test_coz: build/$(TEA_TEST_NAME)_coz build/$(MD5_TEST_NAME)_coz build/$(RC4_TEST_NAME)_coz build/$(AES_TEST_NAME)_coz build/$(SHA1_TEST_NAME)_coz

build/$(TEA_TEST_NAME)_co0: build/TEA.co0 build/test_tea.co0 build/printBuffer.co0
	$(CLANG_CC) -o $@ $^
build/$(MD5_TEST_NAME)_co0: build/MD5.co0 build/test_md5.co0 build/printBuffer.co0
	$(CLANG_CC) -o $@ $^
build/$(RC4_TEST_NAME)_co0: build/RC4.co0 build/test_rc4.co0 build/printBuffer.co0
	$(CLANG_CC) -o $@ $^
build/$(AES_TEST_NAME)_co0: build/AES.co0 build/test_aes.co0 build/printBuffer.co0
	$(CLANG_CC) -o $@ $^
build/$(SHA1_TEST_NAME)_co0: build/SHA1.co0 build/test_sha1.co0 build/printBuffer.co0
	$(CLANG_CC) -o $@ $^

build/$(TEA_TEST_NAME)_co1: build/TEA.co1 build/test_tea.co0 build/printBuffer.co0
	$(CLANG_CC) -o $@ $^
build/$(MD5_TEST_NAME)_co1: build/MD5.co1 build/test_md5.co0 build/printBuffer.co0
	$(CLANG_CC) -o $@ $^
build/$(RC4_TEST_NAME)_co1: build/RC4.co1 build/test_rc4.co0 build/printBuffer.co0
	$(CLANG_CC) -o $@ $^
build/$(AES_TEST_NAME)_co1: build/AES.co1 build/test_aes.co0 build/printBuffer.co0
	$(CLANG_CC) -o $@ $^
build/$(SHA1_TEST_NAME)_co1: build/SHA1.co1 build/test_sha1.co0 build/printBuffer.co0
	$(CLANG_CC) -o $@ $^

build/$(TEA_TEST_NAME)_co2: build/TEA.co2 build/test_tea.co0 build/printBuffer.co0
	$(CLANG_CC) -o $@ $^
build/$(MD5_TEST_NAME)_co2: build/MD5.co2 build/test_md5.co0 build/printBuffer.co0
	$(CLANG_CC) -o $@ $^
build/$(RC4_TEST_NAME)_co2: build/RC4.co2 build/test_rc4.co0 build/printBuffer.co0
	$(CLANG_CC) -o $@ $^
build/$(AES_TEST_NAME)_co2: build/AES.co2 build/test_aes.co0 build/printBuffer.co0
	$(CLANG_CC) -o $@ $^
build/$(SHA1_TEST_NAME)_co2: build/SHA1.co2 build/test_sha1.co0 build/printBuffer.co0
	$(CLANG_CC) -o $@ $^

build/$(TEA_TEST_NAME)_co3: build/TEA.co3 build/test_tea.co0 build/printBuffer.co0
	$(CLANG_CC) -o $@ $^
build/$(MD5_TEST_NAME)_co3: build/MD5.co3 build/test_md5.co0 build/printBuffer.co0
	$(CLANG_CC) -o $@ $^
build/$(RC4_TEST_NAME)_co3: build/RC4.co3 build/test_rc4.co0 build/printBuffer.co0
	$(CLANG_CC) -o $@ $^
build/$(AES_TEST_NAME)_co3: build/AES.co3 build/test_aes.co0 build/printBuffer.co0
	$(CLANG_CC) -o $@ $^
build/$(SHA1_TEST_NAME)_co3: build/SHA1.co3 build/test_sha1.co0 build/printBuffer.co0
	$(CLANG_CC) -o $@ $^

build/$(TEA_TEST_NAME)_coz: build/TEA.coz build/test_tea.co0 build/printBuffer.co0
	$(CLANG_CC) -o $@ $^
build/$(MD5_TEST_NAME)_coz: build/MD5.coz build/test_md5.co0 build/printBuffer.co0
	$(CLANG_CC) -o $@ $^
build/$(RC4_TEST_NAME)_coz: build/RC4.coz build/test_rc4.co0 build/printBuffer.co0
	$(CLANG_CC) -o $@ $^
build/$(AES_TEST_NAME)_coz: build/AES.coz build/test_aes.co0 build/printBuffer.co0
	$(CLANG_CC) -o $@ $^
build/$(SHA1_TEST_NAME)_coz: build/SHA1.coz build/test_sha1.co0 build/printBuffer.co0
	$(CLANG_CC) -o $@ $^

build/%.co0: src/%.c
	$(CLANG_CC) -o $@ -c $< $(CLANG_CFLAGS) -g -I$(MISC_DIRECTORY) -I../

build/%.co0: $(MISC_DIRECTORY)%.c
	$(CLANG_CC) -o $@ -c $< $(CLANG_CFLAGS) -g

build/%.co0: ../%.c
	$(CLANG_CC) -o $@ -c $< $(CLANG_CFLAGS) -O0

build/%.co1: ../%.c
	$(CLANG_CC) -o $@ -c $< $(CLANG_CFLAGS) -O1

build/%.co2: ../%.c
	$(CLANG_CC) -o $@ -c $< $(CLANG_CFLAGS) -O2

build/%.co3: ../%.c
	$(CLANG_CC) -o $@ -c $< $(CLANG_CFLAGS) -O3

build/%.coz: ../%.c
	$(CLANG_CC) -o $@ -c $< $(CLANG_CFLAGS) -Oz

clean:
	@ rm -f build/*

.PHONY: all clean