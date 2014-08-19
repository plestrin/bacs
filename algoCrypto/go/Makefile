MD5_GO_EXAMPLE_NAME 	:= go_md5
SHA1_GO_EXAMPLE_NAME 	:= go_sha1
RC4_GO_EXAMPLE_NAME 	:= go_rc4

all: build/$(MD5_GO_EXAMPLE_NAME) build/$(SHA1_GO_EXAMPLE_NAME) build/$(RC4_GO_EXAMPLE_NAME)

build/$(MD5_GO_EXAMPLE_NAME): src/go_md5.go
	go build -o $@ $^

build/$(SHA1_GO_EXAMPLE_NAME): src/go_sha1.go
	go build -o $@ $^

build/$(RC4_GO_EXAMPLE_NAME): src/go_rc4.go
	go build -o $@ $^

clean:
	@ rm -f build/*

.PHONY: all clean


