TEST_DIRECTORY			:= test/
TOMCRYPT_DIRECTORY 		:= tomcrypt/
CRYPTOPP_DIRECTORY 		:= cryptopp/
GLADMAN_DIRECTORY 		:= gladman/
OPENSSL_DIRECTORY 		:= openssl/
BOTAN_DIRECTORY 		:= botan/
NETTLE_DIRECTORY 		:= nettle/

all: test tomCrypt cryptopp gladman openssl go botan nettle

test:
	@ cd $(TEST_DIRECTORY) && $(MAKE)

tomCrypt:
	@ cd $(TOMCRYPT_DIRECTORY) && $(MAKE)

cryptopp:
	@ cd $(CRYPTOPP_DIRECTORY) && $(MAKE)

gladman:
	@ cd $(GLADMAN_DIRECTORY) && $(MAKE)

openssl:
	@ cd $(OPENSSL_DIRECTORY) && $(MAKE)

botan:
	@ cd $(BOTAN_DIRECTORY) && $(MAKE)

nettle:
	@ cd $(NETTLE_DIRECTORY) && $(MAKE)

.PHONY: all clean test tomCrypt cryptopp gladman openssl go botan nettle

clean:
	@ cd $(TOMCRYPT_DIRECTORY) && $(MAKE) clean
	@ cd $(TEST_DIRECTORY) && $(MAKE) clean
	@ cd $(CRYPTOPP_DIRECTORY) && $(MAKE) clean
	@ cd $(GLADMAN_DIRECTORY) && $(MAKE) clean
	@ cd $(OPENSSL_DIRECTORY) && $(MAKE) clean
	@ cd $(GO_DIRECTORY) && $(MAKE) clean
	@ cd $(BOTAN_DIRECTORY) && $(MAKE) clean
	@ cd $(NETTLE_DIRECTORY) && $(MAKE) clean
