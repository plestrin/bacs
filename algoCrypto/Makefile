EXAMPLE_DIRECTORY			:= example/
TOMCRYPT_DIRECTORY 			:= tomCrypt/
DEBUG 						:= 0

CC							:= gcc
ifeq ($(DEBUG),1)
CFLAGS						:= -W -Wall -pedantic -Wextra -g
else
CFLAGS						:= -W -Wall -pedantic -Wextra -O1
endif
LDFLAGS						:=
VPATH						:= $(TEST_DIRECTORY)

TEA_EXAMPLE_NAME			:= TEA
MD5_EXAMPLE_NAME 			:= MD5
RC4_EXAMPLE_NAME 			:= RC4
AES_EXAMPLE_NAME 			:= AES

MD5_TOMCRYPT_EXAMPLE_NAME 	:= MD5
RC4_TOMCRYPT_EXAMPLE_NAME 	:= RC4

all: example tomCrypt

# *** Example compile rules ***
example: tea_example md5_example rc4_example aes_example

tea_example: TEA.o $(EXAMPLE_DIRECTORY)tea_test.o ../misc/printBuffer.o
	$(CC) -o $(EXAMPLE_DIRECTORY)$(TEA_EXAMPLE_NAME) $^ $(LDFLAGS)

md5_example: MD5.o $(EXAMPLE_DIRECTORY)md5_test.o ../misc/printBuffer.o
	$(CC) -o $(EXAMPLE_DIRECTORY)$(MD5_EXAMPLE_NAME) $^ $(LDFLAGS)

rc4_example: RC4.o $(EXAMPLE_DIRECTORY)rc4_test.o ../misc/printBuffer.o
	$(CC) -o $(EXAMPLE_DIRECTORY)$(RC4_EXAMPLE_NAME) $^ $(LDFLAGS)

aes_example: AES.o $(EXAMPLE_DIRECTORY)aes_test.o ../misc/printBuffer.o
	$(CC) -o $(EXAMPLE_DIRECTORY)$(AES_EXAMPLE_NAME) $^ $(LDFLAGS)

# *** TomCrypt compile rules ***
tomCrypt: tomCrypt_md5_example tomCrypt_rc4_example

tomCrypt_md5_example: $(TOMCRYPT_DIRECTORY)md5_tom.o ../misc/printBuffer.o
	$(CC) -o $(TOMCRYPT_DIRECTORY)$(MD5_TOMCRYPT_EXAMPLE_NAME) $^ $(LDFLAGS) -ltomcrypt

tomCrypt_rc4_example: $(TOMCRYPT_DIRECTORY)rc4_tom.o ../misc/printBuffer.o
	$(CC) -o $(TOMCRYPT_DIRECTORY)$(RC4_TOMCRYPT_EXAMPLE_NAME) $^ $(LDFLAGS) -ltomcrypt

# *** General compile rules ***
%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

MD5.o: MD5.h
TEA.o: TEA.h
RC4.o: RC4.h

.PHONY: clean example tomCrypt

clean:
	@ rm -f *.o
	@ rm -f *~
	@ rm -f $(EXAMPLE_DIRECTORY)*.o
	@ rm -f $(EXAMPLE_DIRECTORY)$(TEA_EXAMPLE_NAME)
	@ rm -f $(EXAMPLE_DIRECTORY)$(MD5_EXAMPLE_NAME)
	@ rm -f $(EXAMPLE_DIRECTORY)$(RC4_EXAMPLE_NAME)
	@ rm -f $(EXAMPLE_DIRECTORY)$(AES_EXAMPLE_NAME)
	@ rm -f $(TOMCRYPT_DIRECTORY)*.o
	@ rm -f $(TOMCRYPT_DIRECTORY)$(MD5_TOMCRYPT_EXAMPLE_NAME)
	@ rm -f $(TOMCRYPT_DIRECTORY)$(RC4_TOMCRYPT_EXAMPLE_NAME)


