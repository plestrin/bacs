CC		:= clang
CFLAGS	:= -Weverything -g -O0 -std=gnu99 -DDEBUG -DCOLOR
LDFLAGS	:=
TEST 	:= test_llist test_graph

all: build $(foreach test,$(TEST),build/$(test))

build/test_llist: build/test_llist.o
	$(CC) -o $@ $^ $ $(LDFLAGS)

build/test_graph: build/test_graph.o build/graph.o
	$(CC) -o $@ $^ $ $(LDFLAGS)

build:
	@ mkdir -p build

build/%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

build/%.o: ../%.c
	$(CC) -o $@ -c $< $(CFLAGS)

test: build $(TEST)

define TEMPLATE_TEST
$(1): build/$(1)
	@ valgrind -q ./build/$(1)
endef

$(foreach test,$(TEST),$(eval $(call TEMPLATE_TEST,$(test))))

clean:
	@ rm -rf build

.PHONY: all test clean
