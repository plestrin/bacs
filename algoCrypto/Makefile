EXAMPLE_DIRECTORY	:=example/

CC			:=gcc
CFLAGS			:=-W -Wall -pedantic -Wextra
LDFLAGS			:=
VPATH			:=$(TEST_DIRECTORY)

TEA_EXAMPLE_NAME	:=TEA

all: example

example: tea_example

tea_example: TEA.o $(EXAMPLE_DIRECTORY)tea_test.o
	$(CC) -o $(EXAMPLE_DIRECTORY)$(TEA_EXAMPLE_NAME) $^ $(LDFLAGS)

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

$(EXAMPLE_DIRECTORY)%.o: $(EXAMPLE_DIRECTORY)%.c
	$(CC) -o $@ -c $< $(CFLAGS)

.PHONY: clean example

clean:
	@ rm -f *.o
	@ rm -f *~
	@ rm -f $(EXAMPLE_DIRECTORY)*.o
	@ rm -f $(EXAMPLE_DIRECTORY)$(TEA_EXAMPLE_NAME)
