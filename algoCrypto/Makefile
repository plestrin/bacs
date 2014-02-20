EXAMPLE_DIRECTORY			:= example/
TOMCRYPT_DIRECTORY 			:= tomCrypt/
CRYPTOPP_DIRECTORY 			:= cryptopp/

all: example tomCrypt cryptopp

example:
	@ cd $(EXAMPLE_DIRECTORY) && $(MAKE)

tomCrypt:
	@ cd $(TOMCRYPT_DIRECTORY) && $(MAKE)

cryptopp:
	@ cd $(CRYPTOPP_DIRECTORY) && $(MAKE)

.PHONY: all clean example tomCrypt cryptopp

clean:
	@ cd $(TOMCRYPT_DIRECTORY) && $(MAKE) clean
	@ cd $(EXAMPLE_DIRECTORY) && $(MAKE) clean
	@ cd $(CRYPTOPP_DIRECTORY) && $(MAKE) clean
