package main

import (
	"crypto/rc4"
	"fmt"
	"bytes"
)

func main() {
	plaintext := "Hello World!"
	key := "Key"
	ciphertext := make([]byte, len(plaintext))
	deciphertext := make([]byte, len(plaintext))

	enc_cipher, error := rc4.NewCipher([]byte(key))
	if error == nil {
		fmt.Printf("Plaintext:  \"%s\"\n", plaintext);
		fmt.Printf("Key:        \"%s\"\n", key);

		enc_cipher.XORKeyStream(ciphertext, []byte(plaintext))
		
		fmt.Printf("Ciphertext: %x\n", ciphertext)
		
		dec_cipher, error := rc4.NewCipher([]byte(key))
		if error == nil {
			dec_cipher.XORKeyStream(deciphertext, ciphertext)
			
			if bytes.Equal(deciphertext, []byte(plaintext)) {
				fmt.Printf("Check:      OK\n");
			} else {
				fmt.Printf("Check:      FAIL\n");
			}
		} else {
			fmt.Printf("ERROR: new cipher returns an error code\n")
		}
	} else {
		fmt.Printf("ERROR: new cipher returns an error code\n")
	}
}