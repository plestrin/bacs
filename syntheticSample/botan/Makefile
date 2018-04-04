CXX 		:= g++
CXXFLAGS 	:= -W -Wall -I/usr/include/botan-1.10/
LDFLAGS 	:= -lbotan-1.10

TEA_BOTAN_EXAMPLE_NAME 			:= bot_tea
ECB_TEA_BOTAN_EXAMPLE_NAME 		:= bot_ecb_tea
CBC_TEA_BOTAN_EXAMPLE_NAME 		:= bot_cbc_tea
OFB_TEA_BOTAN_EXAMPLE_NAME 		:= bot_ofb_tea
CFB_TEA_BOTAN_EXAMPLE_NAME 		:= bot_cfb_tea
CTR_TEA_BOTAN_EXAMPLE_NAME 		:= bot_ctr_tea
AES_BOTAN_EXAMPLE_NAME 			:= bot_aes
ECB_AES_BOTAN_EXAMPLE_NAME 		:= bot_ecb_aes
CBC_AES_BOTAN_EXAMPLE_NAME 		:= bot_cbc_aes
OFB_AES_BOTAN_EXAMPLE_NAME 		:= bot_ofb_aes
CFB_AES_BOTAN_EXAMPLE_NAME 		:= bot_cfb_aes
CTR_AES_BOTAN_EXAMPLE_NAME 		:= bot_ctr_aes
SERPENT_BOTAN_EXAMPLE_NAME 		:= bot_serpent
DES_BOTAN_EXAMPLE_NAME 			:= bot_des
RC4_BOTAN_EXAMPLE_NAME 			:= bot_rc4
TWOFISH_BOTAN_EXAMPLE_NAME 		:= bot_twofish
SHA1_BOTAN_EXAMPLE_NAME 		:= bot_sha1
HMAC_SHA1_BOTAN_EXAMPLE_NAME 	:= bot_hmac_sha1
MD5_BOTAN_EXAMPLE_NAME 			:= bot_md5
HMAC_MD5_BOTAN_EXAMPLE_NAME 	:= bot_hmac_md5

all: 	build build/$(TEA_BOTAN_EXAMPLE_NAME) 	\
		build/$(ECB_TEA_BOTAN_EXAMPLE_NAME) 	\
		build/$(CBC_TEA_BOTAN_EXAMPLE_NAME) 	\
		build/$(OFB_TEA_BOTAN_EXAMPLE_NAME) 	\
		build/$(CFB_TEA_BOTAN_EXAMPLE_NAME) 	\
		build/$(CTR_TEA_BOTAN_EXAMPLE_NAME) 	\
		build/$(SERPENT_BOTAN_EXAMPLE_NAME) 	\
		build/$(AES_BOTAN_EXAMPLE_NAME) 		\
		build/$(ECB_AES_BOTAN_EXAMPLE_NAME) 	\
		build/$(CBC_AES_BOTAN_EXAMPLE_NAME) 	\
		build/$(OFB_AES_BOTAN_EXAMPLE_NAME) 	\
		build/$(CFB_AES_BOTAN_EXAMPLE_NAME) 	\
		build/$(CTR_AES_BOTAN_EXAMPLE_NAME) 	\
		build/$(SERPENT_BOTAN_EXAMPLE_NAME) 	\
		build/$(DES_BOTAN_EXAMPLE_NAME) 		\
		build/$(RC4_BOTAN_EXAMPLE_NAME) 		\
		build/$(TWOFISH_BOTAN_EXAMPLE_NAME) 	\
		build/$(SHA1_BOTAN_EXAMPLE_NAME) 		\
		build/$(HMAC_SHA1_BOTAN_EXAMPLE_NAME) 	\
		build/$(MD5_BOTAN_EXAMPLE_NAME) 		\
		build/$(HMAC_MD5_BOTAN_EXAMPLE_NAME)

build:
	mkdir -p build

build/$(TEA_BOTAN_EXAMPLE_NAME): build/bot_tea.o build/misc.o
	$(CXX) -o $@ $^ $(LDFLAGS)

build/$(ECB_TEA_BOTAN_EXAMPLE_NAME): build/bot_ecb_tea.o build/misc.o
	$(CXX) -o $@ $^ $(LDFLAGS)

build/$(CBC_TEA_BOTAN_EXAMPLE_NAME): build/bot_cbc_tea.o build/misc.o
	$(CXX) -o $@ $^ $(LDFLAGS)

build/$(OFB_TEA_BOTAN_EXAMPLE_NAME): build/bot_ofb_tea.o build/misc.o
	$(CXX) -o $@ $^ $(LDFLAGS)

build/$(CFB_TEA_BOTAN_EXAMPLE_NAME): build/bot_cfb_tea.o build/misc.o
	$(CXX) -o $@ $^ $(LDFLAGS)

build/$(CTR_TEA_BOTAN_EXAMPLE_NAME): build/bot_ctr_tea.o build/misc.o
	$(CXX) -o $@ $^ $(LDFLAGS)

build/$(AES_BOTAN_EXAMPLE_NAME): build/bot_aes.o build/misc.o
	$(CXX) -o $@ $^ $(LDFLAGS)

build/$(ECB_AES_BOTAN_EXAMPLE_NAME): build/bot_ecb_aes.o build/misc.o
	$(CXX) -o $@ $^ $(LDFLAGS)

build/$(CBC_AES_BOTAN_EXAMPLE_NAME): build/bot_cbc_aes.o build/misc.o
	$(CXX) -o $@ $^ $(LDFLAGS)

build/$(OFB_AES_BOTAN_EXAMPLE_NAME): build/bot_ofb_aes.o build/misc.o
	$(CXX) -o $@ $^ $(LDFLAGS)

build/$(CFB_AES_BOTAN_EXAMPLE_NAME): build/bot_cfb_aes.o build/misc.o
	$(CXX) -o $@ $^ $(LDFLAGS)

build/$(CTR_AES_BOTAN_EXAMPLE_NAME): build/bot_ctr_aes.o build/misc.o
	$(CXX) -o $@ $^ $(LDFLAGS)

build/$(SERPENT_BOTAN_EXAMPLE_NAME): build/bot_serpent.o build/misc.o
	$(CXX) -o $@ $^ $(LDFLAGS)

build/$(DES_BOTAN_EXAMPLE_NAME): build/bot_des.o build/misc.o
	$(CXX) -o $@ $^ $(LDFLAGS)

build/$(RC4_BOTAN_EXAMPLE_NAME): build/bot_rc4.o build/misc.o
	$(CXX) -o $@ $^ $(LDFLAGS)

build/$(TWOFISH_BOTAN_EXAMPLE_NAME): build/bot_twofish.o build/misc.o
	$(CXX) -o $@ $^ $(LDFLAGS)

build/$(SHA1_BOTAN_EXAMPLE_NAME): build/bot_sha1.o build/misc.o
	$(CXX) -o $@ $^ $(LDFLAGS)

build/$(HMAC_SHA1_BOTAN_EXAMPLE_NAME): build/bot_hmac_sha1.o build/misc.o
	$(CXX) -o $@ $^ $(LDFLAGS)

build/$(MD5_BOTAN_EXAMPLE_NAME): build/bot_md5.o build/misc.o
	$(CXX) -o $@ $^ $(LDFLAGS)

build/$(HMAC_MD5_BOTAN_EXAMPLE_NAME): build/bot_hmac_md5.o build/misc.o
	$(CXX) -o $@ $^ $(LDFLAGS)

build/%.o: src/%.cpp
	$(CXX) -o $@ -c $< $(CXXFLAGS)

clean:
	@ rm -f build/*

.PHONY: all clean
