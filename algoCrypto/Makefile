EXAMPLE_DIRECTORY			:= example/
TOMCRYPT_DIRECTORY 			:= tomCrypt/
CRYPTO++_DIRECTORY 			:= crypto++/

all: example tomCrypt crypto++

example:
	@ cd $(EXAMPLE_DIRECTORY) && $(MAKE)

tomCrypt:
	@ cd $(TOMCRYPT_DIRECTORY) && $(MAKE)

crypto++:
	@ cd $(CRYPTO++_DIRECTORY) && $(MAKE)

.PHONY: all clean example tomCrypt crypto++

clean:
	@ cd $(TOMCRYPT_DIRECTORY) && $(MAKE) clean
	@ cd $(EXAMPLE_DIRECTORY) && $(MAKE) clean
	@ cd $(CRYPTO++_DIRECTORY) && $(MAKE) clean
