include ../../paths.mk

CC		:= gcc
CFLAGS	:= -W -Wall -pedantic -Wextra -O2 -std=gnu99
LDFLAGS	:= -lcrypto
PROG 	:= build/rng
WHL 	:= whiteList.lst
TOOL 	:= ../../lightTracer_pin/obj-ia32/lightTracer.so
ANAL 	:= ../../test/analysis 

all: build $(PROG) trace search

build:
	mkdir -p build

$(PROG): build/rng.o
	$(CC) -o $@ $^ $(LDFLAGS)

build/%.o: src/%.c
	$(CC) -o $@ -c $< $(CFLAGS)

trace: $(WHL) $(PROG)
	@ rm -rf trace
	$(PIN_BIN) -t $(TOOL) -w $(WHL) -m -o trace -- $(PROG)

$(ANAL):
	@ cd $(dir $(ANAL)) && $(MAKE)

search: $(ANAL) trace
	@ $(ANAL) "load code signature ../../reference/codeSignature_sha1.txt" 		\
			"load trace trace" 													\
			"create callGraph LINUX" 											\
			"export callGraph SHA1_Update" 										\
			"export callGraph SHA1_Final" 										\
			"filter frag size 100" 												\
			"create ir" 														\
			"normalize ir" 														\
			"search code signature" 											\
			"export result sha1_compress" 										\
			"export callGraph RAND_add" 										\
			"create compound ir [-5:]" 											\
			"filter frag size 10000" 											\
			"simplify concrete ir" 												\
			"create synthesis" 													\
			"printDot synthesis" 												\
			"exit"

clean:
	@ rm -rf build
	@ rm -rf trace
	@ rm -f *.log
	@ rm -f *.dot
