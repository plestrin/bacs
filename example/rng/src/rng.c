#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include <openssl/rand.h>

#define NB_BYTE_SRC 64
#define NB_BYTE_DST 64

int main(void){
	int file;
	unsigned char buffer_src[NB_BYTE_SRC];
	unsigned char buffer_dst[NB_BYTE_DST];

	if ((file = open("/dev/urandom", O_RDONLY)) < 0){
		printf("ERROR: unable to open file: /dev/urandom\n");
		return EXIT_FAILURE;
	}

	if (read(file, buffer_src, NB_BYTE_SRC) != NB_BYTE_SRC){
		printf("ERROR: unable to read file /dev/urandom\n");
		return EXIT_FAILURE;
	}

	close(file);

	RAND_add(buffer_src, NB_BYTE_SRC, 256);
	RAND_bytes(buffer_dst, NB_BYTE_DST);

	if ((file = open("/dev/null", O_WRONLY)) < 0){
		printf("ERROR: unable to open file: /dev/null\n");
		return EXIT_FAILURE;
	}

	if (write(file, buffer_src, NB_BYTE_SRC) != NB_BYTE_SRC){
		printf("ERROR: unable to write file /dev/null\n");
		return EXIT_FAILURE;
	}

	close(file);

	return EXIT_SUCCESS;
}