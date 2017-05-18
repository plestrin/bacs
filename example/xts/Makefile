include ../../paths.mk

CC		:= gcc
CFLAGS	:= -W -Wall -pedantic -Wextra -O2 -std=gnu99
LDFLAGS	:= -ltomcrypt
PROG 	:= build/xts
WHL 	:= whiteList.lst
TOOL 	:= ../../lightTracer_pin/obj-ia32/lightTracer.so
ANAL 	:= ../../test/analysis

all: build $(PROG) trace search

build:
	mkdir -p build

$(PROG): build/xts.o build/printBuffer.o
	$(CC) -o $@ $^ $(LDFLAGS)

build/%.o: src/%.c
	$(CC) -o $@ -c $< $(CFLAGS)

trace: $(WHL) $(PROG)
	@ rm -rf trace
	$(PIN_BIN) -t $(TOOL) -w $(WHL) -m -o trace -- $(PROG)

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
			"export callGraph xts_encrypt" 										\
			"create compound ir [-1:]" 											\
			"simplify concrete ir [-1:]" 										\
			"create synthesis [-1:]" 											\
			"printDot synthesis 4" 												\
			"exit"

.PHONY: $(ANAL)

clean:
	@ rm -rf build
	@ rm -rf trace
	@ rm -f *.log
	@ rm -f *.dot
