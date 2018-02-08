# Installation Requirements

## LightTracer PIN
* PIN
* XED
* For Windows: Visual Studio Express 2013 MSVC 18.00, GNU Make, PIN version 2.14

If you get an error about `BBL_InsertFillBuffer` while compiling LightTracer PIN, add the following prototype to `$PIN_ROOT)/source/include/pin/gen/pin_client.PH`:
```C
	extern VOID BBL_InsertFillBuffer(BBL bbl, IPOINT action, BUFFER_ID id, ...);
```

## Static Signature
* libelf-dev
* libyajl-dev
* XED

## Assembly Test Programs
* nasm

## Documentation
* pandoc

## Test Samples
* nettle-dev
* libbotan1.10-dev
* libcrypto++-dev
* libtomcrypt-dev
* libssl-dev
