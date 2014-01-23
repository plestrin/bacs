EXAMPLE_DIRECTORY			:= example/
TOMCRYPT_DIRECTORY 			:= tomCrypt/

CC							:= gcc
CFLAGS_DEBUG				:= -W -Wall -pedantic -Wextra -g
CFLAGS_O1					:= -W -Wall -pedantic -Wextra -O1
CFLAGS_O2					:= -W -Wall -pedantic -Wextra -O2
CFLAGS_O3					:= -W -Wall -pedantic -Wextra -O3
CFLAGS_Os					:= -W -Wall -pedantic -Wextra -Os
LDFLAGS						:=
VPATH						:= $(TEST_DIRECTORY)

TEA_EXAMPLE_NAME			:= TEA
MD5_EXAMPLE_NAME 			:= MD5
RC4_EXAMPLE_NAME 			:= RC4
AES_EXAMPLE_NAME 			:= AES

MD5_TOMCRYPT_EXAMPLE_NAME 	:= MD5
RC4_TOMCRYPT_EXAMPLE_NAME 	:= RC4

all: example_o0 example_o1 example_o2 example_o3 example_os tomCrypt

# *** Example compile rules ***
example_o0: tea_example_o0 md5_example_o0 rc4_example_o0 aes_example_o0
example_o1: tea_example_o1 md5_example_o1 rc4_example_o1 aes_example_o1
example_o2: tea_example_o2 md5_example_o2 rc4_example_o2 aes_example_o2
example_o3: tea_example_o3 md5_example_o3 rc4_example_o3 aes_example_o3
example_os: tea_example_os md5_example_os rc4_example_os aes_example_os

tea_example_o0: TEA.o0 $(EXAMPLE_DIRECTORY)tea_test.o0 ../misc/printBuffer.o
	$(CC) -o $(EXAMPLE_DIRECTORY)$(TEA_EXAMPLE_NAME)_o0 $^ $(LDFLAGS)
md5_example_o0: MD5.o0 $(EXAMPLE_DIRECTORY)md5_test.o0 ../misc/printBuffer.o
	$(CC) -o $(EXAMPLE_DIRECTORY)$(MD5_EXAMPLE_NAME)_o0 $^ $(LDFLAGS)
rc4_example_o0: RC4.o0 $(EXAMPLE_DIRECTORY)rc4_test.o0 ../misc/printBuffer.o
	$(CC) -o $(EXAMPLE_DIRECTORY)$(RC4_EXAMPLE_NAME)_o0 $^ $(LDFLAGS)
aes_example_o0: AES.o0 $(EXAMPLE_DIRECTORY)aes_test.o0 ../misc/printBuffer.o
	$(CC) -o $(EXAMPLE_DIRECTORY)$(AES_EXAMPLE_NAME)_o0 $^ $(LDFLAGS)

tea_example_o1: TEA.o1 $(EXAMPLE_DIRECTORY)tea_test.o1 ../misc/printBuffer.o
	$(CC) -o $(EXAMPLE_DIRECTORY)$(TEA_EXAMPLE_NAME)_o1 $^ $(LDFLAGS)
md5_example_o1: MD5.o1 $(EXAMPLE_DIRECTORY)md5_test.o1 ../misc/printBuffer.o
	$(CC) -o $(EXAMPLE_DIRECTORY)$(MD5_EXAMPLE_NAME)_o1 $^ $(LDFLAGS)
rc4_example_o1: RC4.o1 $(EXAMPLE_DIRECTORY)rc4_test.o1 ../misc/printBuffer.o
	$(CC) -o $(EXAMPLE_DIRECTORY)$(RC4_EXAMPLE_NAME)_o1 $^ $(LDFLAGS)
aes_example_o1: AES.o1 $(EXAMPLE_DIRECTORY)aes_test.o1 ../misc/printBuffer.o
	$(CC) -o $(EXAMPLE_DIRECTORY)$(AES_EXAMPLE_NAME)_o1 $^ $(LDFLAGS)

tea_example_o2: TEA.o2 $(EXAMPLE_DIRECTORY)tea_test.o2 ../misc/printBuffer.o
	$(CC) -o $(EXAMPLE_DIRECTORY)$(TEA_EXAMPLE_NAME)_o2 $^ $(LDFLAGS)
md5_example_o2: MD5.o2 $(EXAMPLE_DIRECTORY)md5_test.o2 ../misc/printBuffer.o
	$(CC) -o $(EXAMPLE_DIRECTORY)$(MD5_EXAMPLE_NAME)_o2 $^ $(LDFLAGS)
rc4_example_o2: RC4.o2 $(EXAMPLE_DIRECTORY)rc4_test.o2 ../misc/printBuffer.o
	$(CC) -o $(EXAMPLE_DIRECTORY)$(RC4_EXAMPLE_NAME)_o2 $^ $(LDFLAGS)
aes_example_o2: AES.o2 $(EXAMPLE_DIRECTORY)aes_test.o2 ../misc/printBuffer.o
	$(CC) -o $(EXAMPLE_DIRECTORY)$(AES_EXAMPLE_NAME)_o2 $^ $(LDFLAGS)

tea_example_o3: TEA.o3 $(EXAMPLE_DIRECTORY)tea_test.o3 ../misc/printBuffer.o
	$(CC) -o $(EXAMPLE_DIRECTORY)$(TEA_EXAMPLE_NAME)_o3 $^ $(LDFLAGS)
md5_example_o3: MD5.o3 $(EXAMPLE_DIRECTORY)md5_test.o3 ../misc/printBuffer.o
	$(CC) -o $(EXAMPLE_DIRECTORY)$(MD5_EXAMPLE_NAME)_o3 $^ $(LDFLAGS)
rc4_example_o3: RC4.o3 $(EXAMPLE_DIRECTORY)rc4_test.o3 ../misc/printBuffer.o
	$(CC) -o $(EXAMPLE_DIRECTORY)$(RC4_EXAMPLE_NAME)_o3 $^ $(LDFLAGS)
aes_example_o3: AES.o3 $(EXAMPLE_DIRECTORY)aes_test.o3 ../misc/printBuffer.o
	$(CC) -o $(EXAMPLE_DIRECTORY)$(AES_EXAMPLE_NAME)_o3 $^ $(LDFLAGS)

tea_example_os: TEA.os $(EXAMPLE_DIRECTORY)tea_test.os ../misc/printBuffer.o
	$(CC) -o $(EXAMPLE_DIRECTORY)$(TEA_EXAMPLE_NAME)_os $^ $(LDFLAGS)
md5_example_os: MD5.os $(EXAMPLE_DIRECTORY)md5_test.os ../misc/printBuffer.o
	$(CC) -o $(EXAMPLE_DIRECTORY)$(MD5_EXAMPLE_NAME)_os $^ $(LDFLAGS)
rc4_example_os: RC4.os $(EXAMPLE_DIRECTORY)rc4_test.os ../misc/printBuffer.o
	$(CC) -o $(EXAMPLE_DIRECTORY)$(RC4_EXAMPLE_NAME)_os $^ $(LDFLAGS)
aes_example_os: AES.os $(EXAMPLE_DIRECTORY)aes_test.os ../misc/printBuffer.o
	$(CC) -o $(EXAMPLE_DIRECTORY)$(AES_EXAMPLE_NAME)_os $^ $(LDFLAGS)


# *** TomCrypt compile rules ***
tomCrypt: tomCrypt_md5_example tomCrypt_rc4_example

tomCrypt_md5_example: $(TOMCRYPT_DIRECTORY)md5_tom.o ../misc/printBuffer.o
	$(CC) -o $(TOMCRYPT_DIRECTORY)$(MD5_TOMCRYPT_EXAMPLE_NAME) $^ $(LDFLAGS) -ltomcrypt

tomCrypt_rc4_example: $(TOMCRYPT_DIRECTORY)rc4_tom.o ../misc/printBuffer.o
	$(CC) -o $(TOMCRYPT_DIRECTORY)$(RC4_TOMCRYPT_EXAMPLE_NAME) $^ $(LDFLAGS) -ltomcrypt


# *** General compile rules ***
%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS_DEBUG)

%.o0: %.c
	$(CC) -o $@ -c $< $(CFLAGS_DEBUG)

%.o1: %.c
	$(CC) -o $@ -c $< $(CFLAGS_O1)

%.o2: %.c
	$(CC) -o $@ -c $< $(CFLAGS_O2)

%.o3: %.c
	$(CC) -o $@ -c $< $(CFLAGS_O3)

%.os: %.c
	$(CC) -o $@ -c $< $(CFLAGS_Os)


.PHONY: clean tomCrypt

clean:
	@ rm -f *.o
	@ rm -f *.o0
	@ rm -f *.o1
	@ rm -f *.o2
	@ rm -f *.o3
	@ rm -f *.os
	@ rm -f *~
	@ rm -f $(EXAMPLE_DIRECTORY)*.o
	@ rm -f $(EXAMPLE_DIRECTORY)*.o0
	@ rm -f $(EXAMPLE_DIRECTORY)*.o1
	@ rm -f $(EXAMPLE_DIRECTORY)*.o2
	@ rm -f $(EXAMPLE_DIRECTORY)*.o3
	@ rm -f $(EXAMPLE_DIRECTORY)*.os
	@ rm -f $(EXAMPLE_DIRECTORY)$(TEA_EXAMPLE_NAME)
	@ rm -f $(EXAMPLE_DIRECTORY)$(MD5_EXAMPLE_NAME)
	@ rm -f $(EXAMPLE_DIRECTORY)$(RC4_EXAMPLE_NAME)
	@ rm -f $(EXAMPLE_DIRECTORY)$(AES_EXAMPLE_NAME)
	@ rm -f $(TOMCRYPT_DIRECTORY)*.o
	@ rm -f $(TOMCRYPT_DIRECTORY)$(MD5_TOMCRYPT_EXAMPLE_NAME)
	@ rm -f $(TOMCRYPT_DIRECTORY)$(RC4_TOMCRYPT_EXAMPLE_NAME)


