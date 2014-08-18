package main

import (
	"crypto/sha1"
	"fmt"
	"io"
	"bytes"
)

func main() {
	message := "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu"
	expected_hash := []byte {0xa4, 0x9b, 0x24, 0x46, 0xa0, 0x2c, 0x64, 0x5b, 0xf4, 0x19, 0xf9, 0x95, 0xb6, 0x70, 0x91, 0x25, 0x3a, 0x04, 0xa2, 0x59}

	h := sha1.New()
	io.WriteString(h, message)
	hash := h.Sum(nil)

	fmt.Printf("Plaintext: \"%s\"\n", message)
	fmt.Printf("SHA1 hash: %x\n", hash)

	if bytes.Equal(hash, expected_hash) {
		fmt.Printf("Check:     OK\n")
	} else {
		fmt.Printf("Check:     FAIL\n")
	}

}
