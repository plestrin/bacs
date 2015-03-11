package main

import (
	"crypto/sha1"
	"fmt"
	"io"
)

func main() {
	message := "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu"

	h := sha1.New()
	io.WriteString(h, message)
	hash := h.Sum(nil)

	fmt.Printf("Plaintext: \"%s\"\n", message)
	fmt.Printf("SHA1 hash: %x\n", hash)
}
