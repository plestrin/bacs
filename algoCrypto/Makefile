EXAMPLE_DIRECTORY	:=example/

CC					:=gcc
CFLAGS				:=-W -Wall -pedantic -Wextra -g
LDFLAGS				:=
VPATH				:=$(TEST_DIRECTORY)

TEA_EXAMPLE_NAME	:=TEA
MD5_EXAMPLE_NAME 	:=MD5

all: example

example: tea_example md5_example

tea_example: TEA.o $(EXAMPLE_DIRECTORY)tea_test.o TEA.h
	$(CC) -o $(EXAMPLE_DIRECTORY)$(TEA_EXAMPLE_NAME) $^ $(LDFLAGS)

md5_example: MD5.o TEA.o $(EXAMPLE_DIRECTORY)md5_test.o MD5.h
	$(CC) -o $(EXAMPLE_DIRECTORY)$(MD5_EXAMPLE_NAME) $^ $(LDFLAGS)

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

$(EXAMPLE_DIRECTORY)%.o: $(EXAMPLE_DIRECTORY)%.c
	$(CC) -o $@ -c $< $(CFLAGS)

.PHONY: clean example tea_example md5_example

clean:
	@ rm -f *.o
	@ rm -f *~
	@ rm -f $(EXAMPLE_DIRECTORY)*.o
	@ rm -f $(EXAMPLE_DIRECTORY)$(TEA_EXAMPLE_NAME)
	@ rm -f $(EXAMPLE_DIRECTORY)$(MD5_EXAMPLE_NAME)


