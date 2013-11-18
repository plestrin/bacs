TEST_DIRECTORY=test

CC=gcc
CFLAGS=-W -Wall -pedantic
LDFLAGS=
VPATH=$(TEST_DIRECTORY)

PROG1_NAME=TEA

all: TEA

TEA: TEA.o $(TEST_DIRECTORY)/tea_test.o
	$(CC) -o $(TEST_DIRECTORY)/$(PROG1_NAME) $^ $(LDFLAGS)

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

$(TEST_DIRECTORY)/tea_test.o: $(TEST_DIRECTORY)/tea_test.c
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	rm -f *.o
	rm -f *~
	rm -f $(TEST_DIRECTORY)/*