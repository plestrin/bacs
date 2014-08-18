package main

import (
	"crypto/md5"
	"fmt"
	"io"
	"bytes"
)

func main() {
	message := "12345678901234567890123456789012345678901234567890123456789012345678901234567890"
	expected_hash := []byte{0x57, 0xed, 0xf4, 0xa2, 0x2b, 0xe3, 0xc9, 0x55, 0xac, 0x49, 0xda, 0x2e, 0x21, 0x07, 0xb6, 0x7a}

	h := md5.New()
	io.WriteString(h, message)
	hash := h.Sum(nil)

	fmt.Printf("Plaintext: \"%s\"\n", message)
	fmt.Printf("MD5 hash:  %x\n", hash)

	if bytes.Equal(hash, expected_hash) {
		fmt.Printf("Check:     OK\n")
	} else {
		fmt.Printf("Check:     FAIL\n")
	}

}
