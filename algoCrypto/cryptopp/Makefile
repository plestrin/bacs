CXX								:= g++
CXXFLAGS						:= -W -Wall -pedantic -Wextra -g
LDFLAGS							:= -lcryptopp

TEA_CRYPTOPP_EXAMPLE_NAME 		:= cryptopp_tea
CBC_TEA_CRYPTOPP_EXAMPLE_NAME 	:= cryptopp_cbc_tea
CTR_TEA_CRYPTOPP_EXAMPLE_NAME 	:= cryptopp_ctr_tea
AES_CRYPTOPP_EXAMPLE_NAME 		:= cryptopp_aes
MD5_CRYPTOPP_EXAMPLE_NAME 		:= cryptopp_md5
RC4_CRYPTOPP_EXAMPLE_NAME 		:= cryptopp_rc4
SERPENT_CRYPTOPP_EXAMPLE_NAME 	:= cryptopp_serpent
DES_CRYPTOPP_EXAMPLE_NAME 		:= cryptopp_des
TWOFISH_CRYPTOPP_EXAMPLE_NAME 	:= cryptopp_twofish
SHA1_CRYPTOPP_EXAMPLE_NAME 		:= cryptopp_sha1


all: build/$(TEA_CRYPTOPP_EXAMPLE_NAME) 	\
	 build/$(CBC_TEA_CRYPTOPP_EXAMPLE_NAME) \
	 build/$(CTR_TEA_CRYPTOPP_EXAMPLE_NAME) \
	 build/$(AES_CRYPTOPP_EXAMPLE_NAME) 	\
	 build/$(MD5_CRYPTOPP_EXAMPLE_NAME) 	\
	 build/$(RC4_CRYPTOPP_EXAMPLE_NAME) 	\
	 build/$(SERPENT_CRYPTOPP_EXAMPLE_NAME) \
	 build/$(DES_CRYPTOPP_EXAMPLE_NAME) 	\
	 build/$(TWOFISH_CRYPTOPP_EXAMPLE_NAME) \
	 build/$(SHA1_CRYPTOPP_EXAMPLE_NAME)

build/$(TEA_CRYPTOPP_EXAMPLE_NAME): build/cryptopp_tea.o
	$(CXX) -o $@ $^ $(LDFLAGS)

build/$(CBC_TEA_CRYPTOPP_EXAMPLE_NAME): build/cryptopp_cbc_tea.o
	$(CXX) -o $@ $^ $(LDFLAGS)

build/$(CTR_TEA_CRYPTOPP_EXAMPLE_NAME): build/cryptopp_ctr_tea.o
	$(CXX) -o $@ $^ $(LDFLAGS)

build/$(AES_CRYPTOPP_EXAMPLE_NAME): build/cryptopp_aes.o
	$(CXX) -o $@ $^ $(LDFLAGS)

build/$(MD5_CRYPTOPP_EXAMPLE_NAME): build/cryptopp_md5.o
	$(CXX) -o $@ $^ $(LDFLAGS)

build/$(RC4_CRYPTOPP_EXAMPLE_NAME): build/cryptopp_rc4.o
	$(CXX) -o $@ $^ $(LDFLAGS)

build/$(SERPENT_CRYPTOPP_EXAMPLE_NAME): build/cryptopp_serpent.o
	$(CXX) -o $@ $^ $(LDFLAGS)

build/$(DES_CRYPTOPP_EXAMPLE_NAME): build/cryptopp_des.o
	$(CXX) -o $@ $^ $(LDFLAGS)

build/$(TWOFISH_CRYPTOPP_EXAMPLE_NAME): build/cryptopp_twofish.o
	$(CXX) -o $@ $^ $(LDFLAGS)

build/$(SHA1_CRYPTOPP_EXAMPLE_NAME): build/cryptopp_sha1.o
	$(CXX) -o $@ $^ $(LDFLAGS)

build/%.o: src/%.cpp
	$(CXX) -o $@ -c $< $(CXXFLAGS)

clean:
	@ rm -f build/*

.PHONY: all clean
	