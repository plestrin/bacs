include ../../paths.mk

CC		:= gcc
CFLAGS	:= -W -Wall -pedantic -Wextra -O2 -std=gnu99
LDFLAGS	:=
PROG 	:= build/asa
WHL 	:= whiteList.lst
TOOL 	:= ../../lightTracer_pin/obj-ia32/lightTracer.so
ANAL 	:= ../../test/analysis 

all: build $(PROG) trace search

build:
	mkdir -p build

$(PROG): build/AES.o build/asa.o build/mode.o
	$(CC) -o $@ $^ $(LDFLAGS)

build/%.o: src/%.c
	$(CC) -o $@ -c $< $(CFLAGS)

trace: $(WHL) $(PROG)
	@ rm -rf trace
	$(PIN_BIN) -t $(TOOL) -w $(WHL) -m -o trace -- $(PROG)

$(WHL):
	echo $$(readlink -f $(PROG)) > $(WHL)

search: $(ANAL) trace
	@ $(ANAL) "load code signature ../../reference/codeSignature_aes.txt" 		\
			"load code signature ../../reference/codeSignature_aes128.txt" 		\
			"load trace trace" 													\
			"create callGraph LINUX" 											\
			"export callGraph aes128_encrypt" 									\
			"export callGraph aes128_key_expand_encrypt" 						\
			"create ir" 														\
			"normalize ir" 														\
			"search code signature" 											\
			"export result aes128_v1 ks_aes_128" 								\
			"export callGraph main" 											\
			"create compound ir 6" 												\
			"simplify concrete ir 6" 											\
			"create synthesis 6" 												\
			"printDot synthesis 6" 												\
			"exit"

clean:
	@ rm -rf $(WHL)
	@ rm -rf build
	@ rm -rf trace
	@ rm -f *.log
	@ rm -f *.dot
