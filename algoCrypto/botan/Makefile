CXX							:= g++
CXXFLAGS					:= -W -Wall -I/usr/include/botan-1.10/
LDFLAGS						:= -lbotan-1.10 -ldl

TEA_BOTAN_EXAMPLE_NAME 		:= bot_tea
ECB_TEA_BOTAN_EXAMPLE_NAME 	:= bot_ecb_tea
CBC_TEA_BOTAN_EXAMPLE_NAME 	:= bot_cbc_tea
OFB_TEA_BOTAN_EXAMPLE_NAME 	:= bot_ofb_tea
CFB_TEA_BOTAN_EXAMPLE_NAME 	:= bot_cfb_tea
AES_BOTAN_EXAMPLE_NAME 		:= bot_aes
ECB_AES_BOTAN_EXAMPLE_NAME 	:= bot_ecb_aes
CBC_AES_BOTAN_EXAMPLE_NAME 	:= bot_cbc_aes
OFB_AES_BOTAN_EXAMPLE_NAME 	:= bot_ofb_aes
CFB_AES_BOTAN_EXAMPLE_NAME 	:= bot_cfb_aes
SERPENT_BOTAN_EXAMPLE_NAME 	:= bot_serpent
DES_BOTAN_EXAMPLE_NAME 		:= bot_des
RC4_BOTAN_EXAMPLE_NAME 		:= bot_rc4
TWOFISH_BOTAN_EXAMPLE_NAME 	:= bot_twofish
SHA1_BOTAN_EXAMPLE_NAME 	:= bot_sha1

all: 	build/$(TEA_BOTAN_EXAMPLE_NAME) 		\
		build/$(ECB_TEA_BOTAN_EXAMPLE_NAME) 	\
		build/$(CBC_TEA_BOTAN_EXAMPLE_NAME) 	\
		build/$(OFB_TEA_BOTAN_EXAMPLE_NAME) 	\
		build/$(CFB_TEA_BOTAN_EXAMPLE_NAME) 	\
		build/$(SERPENT_BOTAN_EXAMPLE_NAME) 	\
		build/$(AES_BOTAN_EXAMPLE_NAME) 		\
		build/$(ECB_AES_BOTAN_EXAMPLE_NAME) 	\
		build/$(CBC_AES_BOTAN_EXAMPLE_NAME) 	\
		build/$(OFB_AES_BOTAN_EXAMPLE_NAME) 	\
		build/$(CFB_AES_BOTAN_EXAMPLE_NAME) 	\
		build/$(SERPENT_BOTAN_EXAMPLE_NAME) 	\
		build/$(DES_BOTAN_EXAMPLE_NAME) 		\
		build/$(RC4_BOTAN_EXAMPLE_NAME) 		\
		build/$(TWOFISH_BOTAN_EXAMPLE_NAME) 	\
		build/$(SHA1_BOTAN_EXAMPLE_NAME)

build/$(TEA_BOTAN_EXAMPLE_NAME): build/botan_tea.o
	$(CXX) -o $@ $^ $(LDFLAGS)

build/$(ECB_TEA_BOTAN_EXAMPLE_NAME): build/botan_ecb_tea.o
	$(CXX) -o $@ $^ $(LDFLAGS)

build/$(CBC_TEA_BOTAN_EXAMPLE_NAME): build/botan_cbc_tea.o
	$(CXX) -o $@ $^ $(LDFLAGS)

build/$(OFB_TEA_BOTAN_EXAMPLE_NAME): build/botan_ofb_tea.o
	$(CXX) -o $@ $^ $(LDFLAGS)

build/$(CFB_TEA_BOTAN_EXAMPLE_NAME): build/botan_cfb_tea.o
	$(CXX) -o $@ $^ $(LDFLAGS)

build/$(CTR_TEA_BOTAN_EXAMPLE_NAME): build/botan_ctr_tea.o
	$(CXX) -o $@ $^ $(LDFLAGS)

build/$(AES_BOTAN_EXAMPLE_NAME): build/botan_aes.o
	$(CXX) -o $@ $^ $(LDFLAGS)

build/$(ECB_AES_BOTAN_EXAMPLE_NAME): build/botan_ecb_aes.o
	$(CXX) -o $@ $^ $(LDFLAGS)

build/$(CBC_AES_BOTAN_EXAMPLE_NAME): build/botan_cbc_aes.o
	$(CXX) -o $@ $^ $(LDFLAGS)

build/$(OFB_AES_BOTAN_EXAMPLE_NAME): build/botan_ofb_aes.o
	$(CXX) -o $@ $^ $(LDFLAGS)

build/$(CFB_AES_BOTAN_EXAMPLE_NAME): build/botan_cfb_aes.o
	$(CXX) -o $@ $^ $(LDFLAGS)

build/$(CTR_AES_BOTAN_EXAMPLE_NAME): build/botan_ctr_aes.o
	$(CXX) -o $@ $^ $(LDFLAGS)

build/$(SERPENT_BOTAN_EXAMPLE_NAME): build/botan_serpent.o
	$(CXX) -o $@ $^ $(LDFLAGS)

build/$(DES_BOTAN_EXAMPLE_NAME): build/botan_des.o
	$(CXX) -o $@ $^ $(LDFLAGS)

build/$(RC4_BOTAN_EXAMPLE_NAME): build/botan_rc4.o
	$(CXX) -o $@ $^ $(LDFLAGS)

build/$(TWOFISH_BOTAN_EXAMPLE_NAME): build/botan_twofish.o
	$(CXX) -o $@ $^ $(LDFLAGS)

build/$(SHA1_BOTAN_EXAMPLE_NAME): build/botan_sha1.o
	$(CXX) -o $@ $^ $(LDFLAGS)

build/%.o: src/%.cpp
	$(CXX) -o $@ -c $< $(CXXFLAGS)

clean:
	@ rm -f build/*

.PHONY: all clean
	