package main

import (
	"crypto/md5"
	"fmt"
	"io"
)

func main() {
	message := "12345678901234567890123456789012345678901234567890123456789012345678901234567890"

	h := md5.New()
	io.WriteString(h, message)
	hash := h.Sum(nil)

	fmt.Printf("Plaintext: \"%s\"\n", message)
	fmt.Printf("MD5 hash:  %x\n", hash)
}
