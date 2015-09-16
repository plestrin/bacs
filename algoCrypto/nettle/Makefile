MISC_DIRECTORY 					:= /home/pierre/Documents/bacs/misc/

CC								:= gcc
CFLAGS							:= -W -Wall -pedantic -Wextra -I$(MISC_DIRECTORY) -g
LDFLAGS							:= -lnettle

AES_NETTLE_EXAMPLE_NAME 		:= net_aes
CBC_AES_NETTLE_EXAMPLE_NAME 	:= net_cbc_aes
CTR_AES_NETTLE_EXAMPLE_NAME 	:= net_ctr_aes

all: build/$(AES_NETTLE_EXAMPLE_NAME) 			\
	 build/$(CBC_AES_NETTLE_EXAMPLE_NAME) 		\
	 build/$(CTR_AES_NETTLE_EXAMPLE_NAME)

build/$(AES_NETTLE_EXAMPLE_NAME): build/nettle_aes.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

build/$(CBC_AES_NETTLE_EXAMPLE_NAME): build/nettle_cbc_aes.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

build/$(CTR_AES_NETTLE_EXAMPLE_NAME): build/nettle_ctr_aes.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

build/%.o: src/%.c
	$(CC) -o $@ -c $< $(CFLAGS)

build/%.o: $(MISC_DIRECTORY)%.c
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	@ rm -f build/*

.PHONY: all clean


