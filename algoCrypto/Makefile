TEST_DIRECTORY			:= test/
TOMCRYPT_DIRECTORY 		:= tomCrypt/
CRYPTOPP_DIRECTORY 		:= cryptopp/
GLADMAN_DIRECTORY 		:= gladman/

all: test tomCrypt cryptopp gladman

test:
	@ cd $(TEST_DIRECTORY) && $(MAKE)

tomCrypt:
	@ cd $(TOMCRYPT_DIRECTORY) && $(MAKE)

cryptopp:
	@ cd $(CRYPTOPP_DIRECTORY) && $(MAKE)

gladman:
	@ cd $(GLADMAN_DIRECTORY) && $(MAKE)

.PHONY: all clean example tomCrypt cryptopp gladman

clean:
	@ cd $(TOMCRYPT_DIRECTORY) && $(MAKE) clean
	@ cd $(TEST_DIRECTORY) && $(MAKE) clean
	@ cd $(CRYPTOPP_DIRECTORY) && $(MAKE) clean
	@ cd $(GLADMAN_DIRECTORY) && $(MAKE) clean
