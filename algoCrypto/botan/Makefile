CXX							:= g++
CXXFLAGS					:= -W -Wall -I/usr/include/botan-1.10/
LDFLAGS						:= -lbotan-1.10

TEA_BOTAN_EXAMPLE_NAME 		:= bot_tea
AES_BOTAN_EXAMPLE_NAME 		:= bot_aes
SERPENT_BOTAN_EXAMPLE_NAME 	:= bot_serpent
DES_BOTAN_EXAMPLE_NAME 		:= bot_des
RC4_BOTAN_EXAMPLE_NAME 		:= bot_rc4
TWOFISH_BOTAN_EXAMPLE_NAME 	:= bot_twofish
SHA1_BOTAN_EXAMPLE_NAME 	:= bot_sha1

all: 	build/$(TEA_BOTAN_EXAMPLE_NAME) 		\
		build/$(AES_BOTAN_EXAMPLE_NAME) 		\
		build/$(SERPENT_BOTAN_EXAMPLE_NAME) 	\
		build/$(DES_BOTAN_EXAMPLE_NAME) 		\
		build/$(RC4_BOTAN_EXAMPLE_NAME) 		\
		build/$(TWOFISH_BOTAN_EXAMPLE_NAME) 	\
		build/$(SHA1_BOTAN_EXAMPLE_NAME)

build/$(TEA_BOTAN_EXAMPLE_NAME): build/botan_tea.o
	$(CXX) -o $@ $^ $(LDFLAGS)

build/$(AES_BOTAN_EXAMPLE_NAME): build/botan_aes.o
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
	