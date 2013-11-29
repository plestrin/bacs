EXAMPLE_DIRECTORY	:= example/

CC					:= gcc
CFLAGS				:= -W -Wall -pedantic -Wextra -g
LDFLAGS				:=
VPATH				:= $(TEST_DIRECTORY)

TEA_EXAMPLE_NAME	:= TEA
MD5_EXAMPLE_NAME 	:= MD5
RC4_EXAMPLE_NAME 	:= RC4

all: example

example: tea_example md5_example rc4_example

tea_example: TEA.o $(EXAMPLE_DIRECTORY)tea_test.o ../misc/printBuffer.o
	$(CC) -o $(EXAMPLE_DIRECTORY)$(TEA_EXAMPLE_NAME) $^ $(LDFLAGS)

md5_example: MD5.o $(EXAMPLE_DIRECTORY)md5_test.o ../misc/printBuffer.o
	$(CC) -o $(EXAMPLE_DIRECTORY)$(MD5_EXAMPLE_NAME) $^ $(LDFLAGS)

rc4_example: RC4.o $(EXAMPLE_DIRECTORY)rc4_test.o ../misc/printBuffer.o
	$(CC) -o $(EXAMPLE_DIRECTORY)$(RC4_EXAMPLE_NAME) $^ $(LDFLAGS)

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

MD5.o: MD5.h
TEA.o: TEA.h
RC4.o: RC4.h

.PHONY: clean example

clean:
	@ rm -f *.o
	@ rm -f *~
	@ rm -f $(EXAMPLE_DIRECTORY)*.o
	@ rm -f $(EXAMPLE_DIRECTORY)$(TEA_EXAMPLE_NAME)
	@ rm -f $(EXAMPLE_DIRECTORY)$(MD5_EXAMPLE_NAME)
	@ rm -f $(EXAMPLE_DIRECTORY)$(RC4_EXAMPLE_NAME)


