#!/usr/bin/python

from Crypto.Cipher import AES

msg = "Hi I am an AES CBC test vector distributed on 4 128-bit blocks!\0"
key = "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f"
iv  = "\x01\xff\x83\xf2\xf9\x98\xba\xa4\xda\xdc\xaa\xcc\x8e\x17\xa4\x1b"

print "Plaintext:      \"" + msg + "\""
print "IV:             " + iv.encode("hex")
print "Key 128:        " + key.encode("hex")

aes = AES.new(key, AES.MODE_CBC, iv)
ct = aes.encrypt(msg)

print "Ciphertext CBC: " + ct.encode("hex")

aes = AES.new(key, AES.MODE_CBC, iv)
dt = aes.decrypt(ct)

if dt == msg:
	print "Recovery:       OK"
else:
	print "Recovery:       FAIL"
