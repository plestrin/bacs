CC		:= gcc
CFLAGS	:= -W -Wall -pedantic -Wextra -O2 -std=gnu99
LDFLAGS	:= -ltomcrypt
PROG 	:= build/ocb
WHL 	:= whiteList.lst
PIN 	:= /home/plt/Documents/tools/pin-3.2-81205-gcc-linux/pin
TOOL 	:= ../../lightTracer_pin/obj-ia32/lightTracer.so
ANAL 	:= ../../test/analysis

all: build $(PROG) trace search

build:
	mkdir -p build

$(PROG): build/ocb.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

build/%.o: src/%.c
	$(CC) -o $@ -c $< $(CFLAGS)

trace: $(WHL) $(PROG)
	@ rm -rf trace
	$(PIN) -t $(TOOL) -w $(WHL) -m -o trace -- $(PROG)

$(ANAL):
	@ cd $(dir $(ANAL)) && $(MAKE)

search: $(ANAL) trace
	@ $(ANAL) "load code signature ../../reference/codeSignature_aes.txt" 		\
			"load code signature ../../reference/codeSignature_aes128.txt" 		\
			"load trace trace" 													\
			"create callGraph LINUX" 											\
			"export callGraph rijndael_ecb_encrypt" 							\
			"create ir" 														\
			"normalize ir" 														\
			"search code signature" 											\
			"export result aes128_v1" 											\
			"export callGraph ocb_encrypt_authenticate_memory" 					\
			"create compound ir [-1:]" 											\
			"simplify concrete ir [-1:]" 										\
			"create synthesis [-1:]" 											\
			"printDot synthesis 8" 												\
			"exit"

.PHONY: $(ANAL)

clean:
	@ rm -rf build
	@ rm -rf trace
	@ rm -f *.log
	@ rm -f *.dot
