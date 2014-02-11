EXAMPLE_DIRECTORY			:= example/
TOMCRYPT_DIRECTORY 			:= tomCrypt/
CRYPTO++_DIRECTORY 			:= crypto++/

CC							:= gcc
CFLAGS_DEBUG				:= -W -Wall -pedantic -Wextra -g
CFLAGS_O1					:= -W -Wall -pedantic -Wextra -O1
CFLAGS_O2					:= -W -Wall -pedantic -Wextra -O2
CFLAGS_O3					:= -W -Wall -pedantic -Wextra -O3
CFLAGS_Os					:= -W -Wall -pedantic -Wextra -Os
LDFLAGS						:=

TEA_EXAMPLE_NAME			:= TEA
MD5_EXAMPLE_NAME 			:= MD5
RC4_EXAMPLE_NAME 			:= RC4
AES_EXAMPLE_NAME 			:= AES
SHA1_EXAMPLE_NAME 			:= SHA1

MD5_TOMCRYPT_EXAMPLE_NAME 	:= MD5
RC4_TOMCRYPT_EXAMPLE_NAME 	:= RC4
AES_TOMCRYPT_EXAMPLE_NAME 	:= AES

all: example_o0 example_o1 example_o2 example_o3 example_os tomCrypt crypto++

# *** Example compile rules ***
example_o0: $(EXAMPLE_DIRECTORY)$(TEA_EXAMPLE_NAME)_o0 $(EXAMPLE_DIRECTORY)$(MD5_EXAMPLE_NAME)_o0 $(EXAMPLE_DIRECTORY)$(RC4_EXAMPLE_NAME)_o0 $(EXAMPLE_DIRECTORY)$(AES_EXAMPLE_NAME)_o0 $(EXAMPLE_DIRECTORY)$(SHA1_EXAMPLE_NAME)_o0
example_o1: $(EXAMPLE_DIRECTORY)$(TEA_EXAMPLE_NAME)_o1 $(EXAMPLE_DIRECTORY)$(MD5_EXAMPLE_NAME)_o1 $(EXAMPLE_DIRECTORY)$(RC4_EXAMPLE_NAME)_o1 $(EXAMPLE_DIRECTORY)$(AES_EXAMPLE_NAME)_o1 $(EXAMPLE_DIRECTORY)$(SHA1_EXAMPLE_NAME)_o1
example_o2: $(EXAMPLE_DIRECTORY)$(TEA_EXAMPLE_NAME)_o2 $(EXAMPLE_DIRECTORY)$(MD5_EXAMPLE_NAME)_o2 $(EXAMPLE_DIRECTORY)$(RC4_EXAMPLE_NAME)_o2 $(EXAMPLE_DIRECTORY)$(AES_EXAMPLE_NAME)_o2 $(EXAMPLE_DIRECTORY)$(SHA1_EXAMPLE_NAME)_o2
example_o3: $(EXAMPLE_DIRECTORY)$(TEA_EXAMPLE_NAME)_o3 $(EXAMPLE_DIRECTORY)$(MD5_EXAMPLE_NAME)_o3 $(EXAMPLE_DIRECTORY)$(RC4_EXAMPLE_NAME)_o3 $(EXAMPLE_DIRECTORY)$(AES_EXAMPLE_NAME)_o3 $(EXAMPLE_DIRECTORY)$(SHA1_EXAMPLE_NAME)_o3
example_os: $(EXAMPLE_DIRECTORY)$(TEA_EXAMPLE_NAME)_os $(EXAMPLE_DIRECTORY)$(MD5_EXAMPLE_NAME)_os $(EXAMPLE_DIRECTORY)$(RC4_EXAMPLE_NAME)_os $(EXAMPLE_DIRECTORY)$(AES_EXAMPLE_NAME)_os $(EXAMPLE_DIRECTORY)$(SHA1_EXAMPLE_NAME)_os

$(EXAMPLE_DIRECTORY)$(TEA_EXAMPLE_NAME)_o0: TEA.o0 $(EXAMPLE_DIRECTORY)tea_test.o0 ../misc/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)
$(EXAMPLE_DIRECTORY)$(MD5_EXAMPLE_NAME)_o0: MD5.o0 $(EXAMPLE_DIRECTORY)md5_test.o0 ../misc/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)
$(EXAMPLE_DIRECTORY)$(RC4_EXAMPLE_NAME)_o0: RC4.o0 $(EXAMPLE_DIRECTORY)rc4_test.o0 ../misc/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)
$(EXAMPLE_DIRECTORY)$(AES_EXAMPLE_NAME)_o0: AES.o0 $(EXAMPLE_DIRECTORY)aes_test.o0 ../misc/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)
$(EXAMPLE_DIRECTORY)$(SHA1_EXAMPLE_NAME)_o0: SHA1.o0 $(EXAMPLE_DIRECTORY)sha1_test.o0 ../misc/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

$(EXAMPLE_DIRECTORY)$(TEA_EXAMPLE_NAME)_o1: TEA.o1 $(EXAMPLE_DIRECTORY)tea_test.o1 ../misc/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)
$(EXAMPLE_DIRECTORY)$(MD5_EXAMPLE_NAME)_o1: MD5.o1 $(EXAMPLE_DIRECTORY)md5_test.o1 ../misc/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)
$(EXAMPLE_DIRECTORY)$(RC4_EXAMPLE_NAME)_o1: RC4.o1 $(EXAMPLE_DIRECTORY)rc4_test.o1 ../misc/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)
$(EXAMPLE_DIRECTORY)$(AES_EXAMPLE_NAME)_o1: AES.o1 $(EXAMPLE_DIRECTORY)aes_test.o1 ../misc/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)
$(EXAMPLE_DIRECTORY)$(SHA1_EXAMPLE_NAME)_o1: SHA1.o1 $(EXAMPLE_DIRECTORY)sha1_test.o1 ../misc/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

$(EXAMPLE_DIRECTORY)$(TEA_EXAMPLE_NAME)_o2: TEA.o2 $(EXAMPLE_DIRECTORY)tea_test.o2 ../misc/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)
$(EXAMPLE_DIRECTORY)$(MD5_EXAMPLE_NAME)_o2: MD5.o2 $(EXAMPLE_DIRECTORY)md5_test.o2 ../misc/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)
$(EXAMPLE_DIRECTORY)$(RC4_EXAMPLE_NAME)_o2: RC4.o2 $(EXAMPLE_DIRECTORY)rc4_test.o2 ../misc/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)
$(EXAMPLE_DIRECTORY)$(AES_EXAMPLE_NAME)_o2: AES.o2 $(EXAMPLE_DIRECTORY)aes_test.o2 ../misc/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)
$(EXAMPLE_DIRECTORY)$(SHA1_EXAMPLE_NAME)_o2: SHA1.o2 $(EXAMPLE_DIRECTORY)sha1_test.o2 ../misc/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

$(EXAMPLE_DIRECTORY)$(TEA_EXAMPLE_NAME)_o3: TEA.o3 $(EXAMPLE_DIRECTORY)tea_test.o3 ../misc/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)
$(EXAMPLE_DIRECTORY)$(MD5_EXAMPLE_NAME)_o3: MD5.o3 $(EXAMPLE_DIRECTORY)md5_test.o3 ../misc/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)
$(EXAMPLE_DIRECTORY)$(RC4_EXAMPLE_NAME)_o3: RC4.o3 $(EXAMPLE_DIRECTORY)rc4_test.o3 ../misc/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)
$(EXAMPLE_DIRECTORY)$(AES_EXAMPLE_NAME)_o3: AES.o3 $(EXAMPLE_DIRECTORY)aes_test.o3 ../misc/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)
$(EXAMPLE_DIRECTORY)$(SHA1_EXAMPLE_NAME)_o3: SHA1.o3 $(EXAMPLE_DIRECTORY)sha1_test.o3 ../misc/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

$(EXAMPLE_DIRECTORY)$(TEA_EXAMPLE_NAME)_os: TEA.os $(EXAMPLE_DIRECTORY)tea_test.os ../misc/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)
$(EXAMPLE_DIRECTORY)$(MD5_EXAMPLE_NAME)_os: MD5.os $(EXAMPLE_DIRECTORY)md5_test.os ../misc/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)
$(EXAMPLE_DIRECTORY)$(RC4_EXAMPLE_NAME)_os: RC4.os $(EXAMPLE_DIRECTORY)rc4_test.os ../misc/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)
$(EXAMPLE_DIRECTORY)$(AES_EXAMPLE_NAME)_os: AES.os $(EXAMPLE_DIRECTORY)aes_test.os ../misc/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)
$(EXAMPLE_DIRECTORY)$(SHA1_EXAMPLE_NAME)_os: SHA1.os $(EXAMPLE_DIRECTORY)sha1_test.os ../misc/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)


# *** TomCrypt compile rules ***
tomCrypt: $(TOMCRYPT_DIRECTORY)$(MD5_TOMCRYPT_EXAMPLE_NAME) $(TOMCRYPT_DIRECTORY)$(RC4_TOMCRYPT_EXAMPLE_NAME) $(TOMCRYPT_DIRECTORY)$(AES_TOMCRYPT_EXAMPLE_NAME)

$(TOMCRYPT_DIRECTORY)$(MD5_TOMCRYPT_EXAMPLE_NAME): $(TOMCRYPT_DIRECTORY)md5_tom.o ../misc/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS) -ltomcrypt

$(TOMCRYPT_DIRECTORY)$(RC4_TOMCRYPT_EXAMPLE_NAME): $(TOMCRYPT_DIRECTORY)rc4_tom.o ../misc/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS) -ltomcrypt

$(TOMCRYPT_DIRECTORY)$(AES_TOMCRYPT_EXAMPLE_NAME): $(TOMCRYPT_DIRECTORY)aes_tom.o ../misc/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS) -ltomcrypt

# *** Crypto++ compile rules ***
crypto++:


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


.PHONY: clean tomCrypt crypto++

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
	@ rm -f $(EXAMPLE_DIRECTORY)$(TEA_EXAMPLE_NAME)_o*
	@ rm -f $(EXAMPLE_DIRECTORY)$(MD5_EXAMPLE_NAME)_o*
	@ rm -f $(EXAMPLE_DIRECTORY)$(RC4_EXAMPLE_NAME)_o*
	@ rm -f $(EXAMPLE_DIRECTORY)$(AES_EXAMPLE_NAME)_o*
	@ rm -f $(EXAMPLE_DIRECTORY)$(SHA1_EXAMPLE_NAME)_o*
	@ rm -f $(TOMCRYPT_DIRECTORY)*.o
	@ rm -f $(TOMCRYPT_DIRECTORY)$(MD5_TOMCRYPT_EXAMPLE_NAME)
	@ rm -f $(TOMCRYPT_DIRECTORY)$(RC4_TOMCRYPT_EXAMPLE_NAME)
	@ rm -f $(TOMCRYPT_DIRECTORY)$(AES_TOMCRYPT_EXAMPLE_NAME)
