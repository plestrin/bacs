MD5_GO_EXAMPLE_NAME 	:= go_md5
SHA1_GO_EXAMPLE_NAME 	:= go_sha1

all: build/$(MD5_GO_EXAMPLE_NAME) build/$(SHA1_GO_EXAMPLE_NAME)

build/$(MD5_GO_EXAMPLE_NAME): src/go_md5.go
	go build -o $@ $^

build/$(SHA1_GO_EXAMPLE_NAME): src/go_sha1.go
	go build -o $@ $^

clean:
	@ rm -f build/*

.PHONY: all clean


