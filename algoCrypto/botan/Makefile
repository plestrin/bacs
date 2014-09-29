CXX								:= g++
CXXFLAGS						:= -W -Wall -I/usr/include/botan-1.10/
LDFLAGS							:= -lbotan-1.10

TEA_BOTAN_EXAMPLE_NAME 			:= bot_tea


all: build/$(TEA_BOTAN_EXAMPLE_NAME)

build/$(TEA_BOTAN_EXAMPLE_NAME): build/botan_tea.o
	$(CXX) -o $@ $^ $(LDFLAGS)

build/%.o: src/%.cpp
	$(CXX) -o $@ -c $< $(CXXFLAGS)

clean:
	@ rm -f build/*

.PHONY: all clean
	