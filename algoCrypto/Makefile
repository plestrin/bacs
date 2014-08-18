TEST_DIRECTORY			:= test/
TOMCRYPT_DIRECTORY 		:= tomCrypt/
CRYPTOPP_DIRECTORY 		:= cryptopp/
GLADMAN_DIRECTORY 		:= gladman/
OPENSSL_DIRECTORY 		:= openssl/
GO_DIRECTORY 			:= go/

all: test tomCrypt cryptopp gladman openssl go

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

go:
	@ cd $(GO_DIRECTORY) && $(MAKE)

.PHONY: all clean test tomCrypt cryptopp gladman openssl go

clean:
	@ cd $(TOMCRYPT_DIRECTORY) && $(MAKE) clean
	@ cd $(TEST_DIRECTORY) && $(MAKE) clean
	@ cd $(CRYPTOPP_DIRECTORY) && $(MAKE) clean
	@ cd $(GLADMAN_DIRECTORY) && $(MAKE) clean
	@ cd $(OPENSSL_DIRECTORY) && $(MAKE) clean
	@ cd $(GO_DIRECTORY) && $(MAKE) clean
