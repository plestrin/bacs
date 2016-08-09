MISC_DIRECTORY 			:= ../../misc/

GCC_CC					:= gcc
GCC_CFLAGS 				:= -W -Wall -pedantic -Wextra -std=gnu99
CLANG_CC 				:= clang
CLANG_CFLAGS 			:= -pedantic -Weverything -Wno-cast-align -Wno-newline-eof -Wno-unused-macros

TEA_TEST_NAME			:= test_tea
ECB_TEA_TEST_NAME 		:= test_ecb_tea
CBC_TEA_TEST_NAME 		:= test_cbc_tea
OFB_TEA_TEST_NAME 		:= test_ofb_tea
CFB_TEA_TEST_NAME 		:= test_cfb_tea
CTR_TEA_TEST_NAME 		:= test_ctr_tea
MD5_TEST_NAME 			:= test_md5
HMAC_MD5_TEST_NAME 		:= test_hmac_md5
RC4_TEST_NAME 			:= test_rc4
AES_TEST_NAME 			:= test_aes
ECB_AES_TEST_NAME 		:= test_ecb_aes
CBC_AES_TEST_NAME 		:= test_cbc_aes
OFB_AES_TEST_NAME 		:= test_ofb_aes
CFB_AES_TEST_NAME 		:= test_cfb_aes
CTR_AES_TEST_NAME 		:= test_ctr_aes
SHA1_TEST_NAME 			:= test_sha1
HMAC_SHA1_TEST_NAME 	:= test_hmac_sha1
SERPENT_TEST_NAME 		:= test_serpent
DES_TEST_NAME 			:= test_des
TWOFISH_TEST_NAME 		:= test_twofish

TARGETS 				:= 	$(TEA_TEST_NAME) 			\
							$(ECB_TEA_TEST_NAME) 		\
							$(CBC_TEA_TEST_NAME) 		\
							$(OFB_TEA_TEST_NAME) 		\
							$(CFB_TEA_TEST_NAME) 		\
							$(CTR_TEA_TEST_NAME) 		\
							$(MD5_TEST_NAME) 			\
							$(HMAC_MD5_TEST_NAME) 		\
							$(RC4_TEST_NAME) 			\
							$(AES_TEST_NAME) 			\
							$(ECB_AES_TEST_NAME) 		\
							$(CBC_AES_TEST_NAME) 		\
							$(OFB_AES_TEST_NAME) 		\
							$(CFB_AES_TEST_NAME) 		\
							$(CTR_AES_TEST_NAME) 		\
							$(SHA1_TEST_NAME) 			\
							$(HMAC_SHA1_TEST_NAME) 		\
							$(SERPENT_TEST_NAME) 		\
							$(DES_TEST_NAME) 			\
							$(TWOFISH_TEST_NAME)

COMPILATIONS 			:= go0 go1 go2 go3 gos co0 co1 co2 co3 coz

define TEMPLATE
build/$$(TEA_TEST_NAME)_$(1): build/TEA.$(1) build/test_tea.$$(shell echo $(1) | tr 0-9sz 0) build/printBuffer.$$(shell echo $(1) | tr 0-9sz 0)
	$$(GCC_CC) -o $$@ $$^
build/$(ECB_TEA_TEST_NAME)_$(1): build/TEA.$(1) build/mode.$(1) build/test_ecb_tea.$$(shell echo $(1) | tr 0-9sz 0) build/printBuffer.$$(shell echo $(1) | tr 0-9sz 0)
	$$(GCC_CC) -o $$@ $$^
build/$(CBC_TEA_TEST_NAME)_$(1): build/TEA.$(1) build/mode.$(1) build/test_cbc_tea.$$(shell echo $(1) | tr 0-9sz 0) build/printBuffer.$$(shell echo $(1) | tr 0-9sz 0)
	$$(GCC_CC) -o $$@ $$^
build/$(OFB_TEA_TEST_NAME)_$(1): build/TEA.$(1) build/mode.$(1) build/test_ofb_tea.$$(shell echo $(1) | tr 0-9sz 0) build/printBuffer.$$(shell echo $(1) | tr 0-9sz 0)
	$$(GCC_CC) -o $$@ $$^
build/$(CFB_TEA_TEST_NAME)_$(1): build/TEA.$(1) build/mode.$(1) build/test_cfb_tea.$$(shell echo $(1) | tr 0-9sz 0) build/printBuffer.$$(shell echo $(1) | tr 0-9sz 0)
	$$(GCC_CC) -o $$@ $$^
build/$(CTR_TEA_TEST_NAME)_$(1): build/TEA.$(1) build/mode.$(1) build/test_ctr_tea.$$(shell echo $(1) | tr 0-9sz 0) build/printBuffer.$$(shell echo $(1) | tr 0-9sz 0)
	$$(GCC_CC) -o $$@ $$^
build/$(MD5_TEST_NAME)_$(1): build/MD5.$(1) build/test_md5.$$(shell echo $(1) | tr 0-9sz 0) build/printBuffer.$$(shell echo $(1) | tr 0-9sz 0)
	$$(GCC_CC) -o $$@ $$^
build/$(HMAC_MD5_TEST_NAME)_$(1): build/MD5.$(1) build/mode.$(1) build/test_hmac_md5.$$(shell echo $(1) | tr 0-9sz 0) build/printBuffer.$$(shell echo $(1) | tr 0-9sz 0)
	$$(GCC_CC) -o $$@ $$^
build/$(RC4_TEST_NAME)_$(1): build/RC4.$(1) build/test_rc4.$$(shell echo $(1) | tr 0-9sz 0) build/printBuffer.$$(shell echo $(1) | tr 0-9sz 0)
	$$(GCC_CC) -o $$@ $$^
build/$(AES_TEST_NAME)_$(1): build/AES.$(1) build/test_aes.$$(shell echo $(1) | tr 0-9sz 0) build/printBuffer.$$(shell echo $(1) | tr 0-9sz 0)
	$$(GCC_CC) -o $$@ $$^
build/$(ECB_AES_TEST_NAME)_$(1): build/AES.$(1) build/mode.$(1) build/test_ecb_aes.$$(shell echo $(1) | tr 0-9sz 0) build/printBuffer.$$(shell echo $(1) | tr 0-9sz 0)
	$$(GCC_CC) -o $$@ $$^
build/$(CBC_AES_TEST_NAME)_$(1): build/AES.$(1) build/mode.$(1) build/test_cbc_aes.$$(shell echo $(1) | tr 0-9sz 0) build/printBuffer.$$(shell echo $(1) | tr 0-9sz 0)
	$$(GCC_CC) -o $$@ $$^
build/$(OFB_AES_TEST_NAME)_$(1): build/AES.$(1) build/mode.$(1) build/test_ofb_aes.$$(shell echo $(1) | tr 0-9sz 0) build/printBuffer.$$(shell echo $(1) | tr 0-9sz 0)
	$$(GCC_CC) -o $$@ $$^
build/$(CFB_AES_TEST_NAME)_$(1): build/AES.$(1) build/mode.$(1) build/test_cfb_aes.$$(shell echo $(1) | tr 0-9sz 0) build/printBuffer.$$(shell echo $(1) | tr 0-9sz 0)
	$$(GCC_CC) -o $$@ $$^
build/$(CTR_AES_TEST_NAME)_$(1): build/AES.$(1) build/mode.$(1) build/test_ctr_aes.$$(shell echo $(1) | tr 0-9sz 0) build/printBuffer.$$(shell echo $(1) | tr 0-9sz 0)
	$$(GCC_CC) -o $$@ $$^
build/$(SHA1_TEST_NAME)_$(1): build/SHA1.$(1) build/test_sha1.$$(shell echo $(1) | tr 0-9sz 0) build/printBuffer.$$(shell echo $(1) | tr 0-9sz 0)
	$$(GCC_CC) -o $$@ $$^
build/$(HMAC_SHA1_TEST_NAME)_$(1): build/SHA1.$(1) build/mode.$(1)  build/test_hmac_sha1.$$(shell echo $(1) | tr 0-9sz 0) build/printBuffer.$$(shell echo $(1) | tr 0-9sz 0)
	$$(GCC_CC) -o $$@ $$^
build/$(SERPENT_TEST_NAME)_$(1): build/serpent.$(1) build/test_serpent.$$(shell echo $(1) | tr 0-9sz 0) build/printBuffer.$$(shell echo $(1) | tr 0-9sz 0)
	$$(GCC_CC) -o $$@ $$^
build/$(DES_TEST_NAME)_$(1): build/DES.$(1) build/test_des.$$(shell echo $(1) | tr 0-9sz 0) build/printBuffer.$$(shell echo $(1) | tr 0-9sz 0)
	$$(GCC_CC) -o $$@ $$^
build/$(TWOFISH_TEST_NAME)_$(1): build/twofish.$(1) build/test_twofish.$$(shell echo $(1) | tr 0-9sz 0) build/printBuffer.$$(shell echo $(1) | tr 0-9sz 0)
	$$(GCC_CC) -o $$@ $$^
endef

all: build $(foreach target,$(TARGETS),$(foreach compilation,$(COMPILATIONS),build/$(target)_$(compilation)))

$(foreach compilation,$(COMPILATIONS),$(eval $(call TEMPLATE,$(compilation))))

build:
	mkdir -p build

# GCC compiler

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
	@ rm -rf build

.PHONY: all clean