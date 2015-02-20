CXX								:= g++
CXXFLAGS						:= -W -Wall -pedantic -Wextra -g
LDFLAGS							:= -lcryptopp

TEA_CRYPTOPP_EXAMPLE_NAME 		:= cryptopp_tea
AES128_CRYPTOPP_EXAMPLE_NAME 	:= cryptopp_aes128
AES192_CRYPTOPP_EXAMPLE_NAME 	:= cryptopp_aes192
AES256_CRYPTOPP_EXAMPLE_NAME 	:= cryptopp_aes256
MD5_CRYPTOPP_EXAMPLE_NAME 		:= cryptopp_md5
RC4_CRYPTOPP_EXAMPLE_NAME 		:= cryptopp_rc4
SERPENT_CRYPTOPP_EXAMPLE_NAME 	:= cryptopp_serpent
DES_CRYPTOPP_EXAMPLE_NAME 		:= cryptopp_des
TWOFISH_CRYPTOPP_EXAMPLE_NAME 	:= cryptopp_twofish


all: build/$(TEA_CRYPTOPP_EXAMPLE_NAME) 	\
	 build/$(AES128_CRYPTOPP_EXAMPLE_NAME) 	\
	 build/$(AES192_CRYPTOPP_EXAMPLE_NAME) 	\
	 build/$(AES256_CRYPTOPP_EXAMPLE_NAME) 	\
	 build/$(MD5_CRYPTOPP_EXAMPLE_NAME) 	\
	 build/$(RC4_CRYPTOPP_EXAMPLE_NAME) 	\
	 build/$(SERPENT_CRYPTOPP_EXAMPLE_NAME) \
	 build/$(DES_CRYPTOPP_EXAMPLE_NAME) 	\
	 build/$(TWOFISH_CRYPTOPP_EXAMPLE_NAME)

build/$(TEA_CRYPTOPP_EXAMPLE_NAME): build/cryptopp_tea.o
	$(CXX) -o $@ $^ $(LDFLAGS)

build/$(AES128_CRYPTOPP_EXAMPLE_NAME): build/cryptopp_aes128.o
	$(CXX) -o $@ $^ $(LDFLAGS)

build/$(AES192_CRYPTOPP_EXAMPLE_NAME): build/cryptopp_aes192.o
	$(CXX) -o $@ $^ $(LDFLAGS)

build/$(AES256_CRYPTOPP_EXAMPLE_NAME): build/cryptopp_aes256.o
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


build/%.o: src/%.cpp
	$(CXX) -o $@ -c $< $(CXXFLAGS)

clean:
	@ rm -f build/*

.PHONY: all clean
	