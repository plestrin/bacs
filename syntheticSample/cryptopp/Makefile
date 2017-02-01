CXX								:= g++
CXXFLAGS						:= -W -Wall -pedantic -Wextra -g
LDFLAGS							:= -lcryptopp

TEA_CRYPTOPP_EXAMPLE_NAME 		:= pp_tea
CBC_TEA_CRYPTOPP_EXAMPLE_NAME 	:= pp_cbc_tea
CTR_TEA_CRYPTOPP_EXAMPLE_NAME 	:= pp_ctr_tea
AES_CRYPTOPP_EXAMPLE_NAME 		:= pp_aes
CBC_AES_CRYPTOPP_EXAMPLE_NAME 	:= pp_cbc_aes
CTR_AES_CRYPTOPP_EXAMPLE_NAME 	:= pp_ctr_aes
MD5_CRYPTOPP_EXAMPLE_NAME 		:= pp_md5
HMAC_MD5_CRYPTOPP_EXAMPLE_NAME 	:= pp_hmac_md5
RC4_CRYPTOPP_EXAMPLE_NAME 		:= pp_rc4
SERPENT_CRYPTOPP_EXAMPLE_NAME 	:= pp_serpent
DES_CRYPTOPP_EXAMPLE_NAME 		:= pp_des
TWOFISH_CRYPTOPP_EXAMPLE_NAME 	:= pp_twofish
SHA1_CRYPTOPP_EXAMPLE_NAME 		:= pp_sha1


all: build build/$(TEA_CRYPTOPP_EXAMPLE_NAME) 	\
	 build/$(CBC_TEA_CRYPTOPP_EXAMPLE_NAME) 	\
	 build/$(CTR_TEA_CRYPTOPP_EXAMPLE_NAME) 	\
	 build/$(AES_CRYPTOPP_EXAMPLE_NAME) 		\
	 build/$(CBC_AES_CRYPTOPP_EXAMPLE_NAME) 	\
	 build/$(CTR_AES_CRYPTOPP_EXAMPLE_NAME) 	\
	 build/$(MD5_CRYPTOPP_EXAMPLE_NAME) 		\
	 build/$(HMAC_MD5_CRYPTOPP_EXAMPLE_NAME) 	\
	 build/$(RC4_CRYPTOPP_EXAMPLE_NAME) 		\
	 build/$(SERPENT_CRYPTOPP_EXAMPLE_NAME) 	\
	 build/$(DES_CRYPTOPP_EXAMPLE_NAME) 		\
	 build/$(TWOFISH_CRYPTOPP_EXAMPLE_NAME) 	\
	 build/$(SHA1_CRYPTOPP_EXAMPLE_NAME)

build:
	mkdir -p build

build/$(TEA_CRYPTOPP_EXAMPLE_NAME): build/pp_tea.o
	$(CXX) -o $@ $^ $(LDFLAGS)

build/$(CBC_TEA_CRYPTOPP_EXAMPLE_NAME): build/pp_cbc_tea.o
	$(CXX) -o $@ $^ $(LDFLAGS)

build/$(CTR_TEA_CRYPTOPP_EXAMPLE_NAME): build/pp_ctr_tea.o
	$(CXX) -o $@ $^ $(LDFLAGS)

build/$(AES_CRYPTOPP_EXAMPLE_NAME): build/pp_aes.o
	$(CXX) -o $@ $^ $(LDFLAGS)

build/$(CBC_AES_CRYPTOPP_EXAMPLE_NAME): build/pp_cbc_aes.o
	$(CXX) -o $@ $^ $(LDFLAGS)

build/$(CTR_AES_CRYPTOPP_EXAMPLE_NAME): build/pp_ctr_aes.o
	$(CXX) -o $@ $^ $(LDFLAGS)

build/$(MD5_CRYPTOPP_EXAMPLE_NAME): build/pp_md5.o
	$(CXX) -o $@ $^ $(LDFLAGS)

build/$(HMAC_MD5_CRYPTOPP_EXAMPLE_NAME): build/pp_hmac_md5.o
	$(CXX) -o $@ $^ $(LDFLAGS)

build/$(RC4_CRYPTOPP_EXAMPLE_NAME): build/pp_rc4.o
	$(CXX) -o $@ $^ $(LDFLAGS)

build/$(SERPENT_CRYPTOPP_EXAMPLE_NAME): build/pp_serpent.o
	$(CXX) -o $@ $^ $(LDFLAGS)

build/$(DES_CRYPTOPP_EXAMPLE_NAME): build/pp_des.o
	$(CXX) -o $@ $^ $(LDFLAGS)

build/$(TWOFISH_CRYPTOPP_EXAMPLE_NAME): build/pp_twofish.o
	$(CXX) -o $@ $^ $(LDFLAGS)

build/$(SHA1_CRYPTOPP_EXAMPLE_NAME): build/pp_sha1.o
	$(CXX) -o $@ $^ $(LDFLAGS)

build/%.o: src/%.cpp
	$(CXX) -o $@ -c $< $(CXXFLAGS)

clean:
	@ rm -f build/*

.PHONY: all clean
	