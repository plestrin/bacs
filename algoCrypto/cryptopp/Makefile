CXX								:= g++
CXXFLAGS						:= -W -Wall -pedantic -Wextra -g
LDFLAGS							:= -lcryptopp

AES128_CRYPTOPP_EXAMPLE_NAME 	:= cryptopp_aes128
AES192_CRYPTOPP_EXAMPLE_NAME 	:= cryptopp_aes192
AES256_CRYPTOPP_EXAMPLE_NAME 	:= cryptopp_aes256


all: build/$(AES128_CRYPTOPP_EXAMPLE_NAME) build/$(AES192_CRYPTOPP_EXAMPLE_NAME) build/$(AES256_CRYPTOPP_EXAMPLE_NAME)

build/$(AES128_CRYPTOPP_EXAMPLE_NAME): build/cryptopp_aes128.o
	$(CXX) -o $@ $^ $(LDFLAGS)

build/$(AES192_CRYPTOPP_EXAMPLE_NAME): build/cryptopp_aes192.o
	$(CXX) -o $@ $^ $(LDFLAGS)

build/$(AES256_CRYPTOPP_EXAMPLE_NAME): build/cryptopp_aes256.o
	$(CXX) -o $@ $^ $(LDFLAGS)

build/%.o: src/%.cpp
	$(CXX) -o $@ -c $< $(CFLAGS)

clean:
	@ rm -f build/*

.PHONY: all clean
	