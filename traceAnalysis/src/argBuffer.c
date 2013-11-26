#include <stdlib.h>
#include <stdio.h>

#include "argBuffer.h"

void argBuffer_print_raw(struct argBuffer* arg){
	unsigned int i;

	if (arg != NULL){
		if (arg->location_type == ARG_LOCATION_MEMORY){
			printf("*** Argument Memory %p ***\n", (void*)arg);
			#pragma GCC diagnostic ignored "-Wformat" /* ISO C90 does not support the ‘ll’ gnu_printf length modifier */
			printf("\t-Address: \t0x%llx\n", arg->location.address);

		}
		else if (arg->location_type == ARG_LOCATION_REGISTER){
			printf("*** Argument Register %p ***\n", (void*)arg);
			/* a completer */
		}
		else{
			printf("ERROR: in %s, incorrect location type in argBuffer\n", __func__);
		}
		
		printf("\t-Size: \t\t%u\n", arg->size);
		printf("\t-Value: \t");

		for (i = 0; i < arg->size; i++){
			printf("%02x", arg->data[i]);
		}

		printf("\n");
	}
}