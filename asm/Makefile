SRC_FILES := $(wildcard src/*.asm)
DST_PROGS := $(addprefix build/,$(notdir $(SRC_FILES:.asm=)))

.PRECIOUS: build/%.o

all: build $(DST_PROGS)

build:
	mkdir -p build

build/%: build/%.o
	gcc -o $@ $^

build/%.o: src/%.asm
	nasm -f elf -o $@ $^

clean:
	@ rm -rf build
