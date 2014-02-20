CXX								:= g++
CXXFLAGS						:= -W -Wall -pedantic -Wextra -g
LDFLAGS							:= -lcryptopp

AES128_CRYPTOPP_EXAMPLE_NAME 	:= AES128
AES192_CRYPTOPP_EXAMPLE_NAME 	:= AES192
AES256_CRYPTOPP_EXAMPLE_NAME 	:= AES256


all: build/$(AES128_CRYPTOPP_EXAMPLE_NAME) build/$(AES192_CRYPTOPP_EXAMPLE_NAME) build/$(AES256_CRYPTOPP_EXAMPLE_NAME)

build/$(AES128_CRYPTOPP_EXAMPLE_NAME): build/aes128_cryptopp.o
	$(CXX) -o $@ $^ $(LDFLAGS)

build/$(AES192_CRYPTOPP_EXAMPLE_NAME): build/aes192_cryptopp.o
	$(CXX) -o $@ $^ $(LDFLAGS)

build/$(AES256_CRYPTOPP_EXAMPLE_NAME): build/aes256_cryptopp.o
	$(CXX) -o $@ $^ $(LDFLAGS)

build/%.o: src/%.cpp
	$(CXX) -o $@ -c $< $(CFLAGS)

clean:
	@ rm -f build/*

.PHONY: all clean
	