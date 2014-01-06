#include <stdlib.h>
#include <stdio.h>

#include "argBuffer.h"
#include "printBuffer.h"

void argBuffer_print_raw(struct argBuffer* arg){
	if (arg != NULL){
		if (arg->location_type == ARG_LOCATION_MEMORY){
			printf("Argument Memory\n");
			#if defined ARCH_32
			printf("\t-Address: \t0x%08x\n", arg->location.address);
			#elif defined ARCH_64
			#pragma GCC diagnostic ignored "-Wformat" /* ISO C90 does not support the ‘ll’ gnu_printf length modifier */
			printf("\t-Address: \t0x%llx\n", arg->location.address);
			#else
			#error Please specify an architecture {ARCH_32 or ARCH_64}
			#endif
		}
		else if (arg->location_type == ARG_LOCATION_REGISTER){
			printf("Argument Register\n");
			/* a completer */
		}
		else{
			printf("ERROR: in %s, incorrect location type in argBuffer\n", __func__);
		}
		
		printf("\t-Size: \t\t%u\n", arg->size);
		printf("\t-Value: \t");

		printBuffer_raw(stdout, arg->data, arg->size);

		printf("\n");
	}
}

void argBuffer_delete_array(struct array* arg_array){
	uint32_t 			i;
	struct argBuffer* 	arg;

	if (arg_array != NULL){
		for (i = 0; i < array_get_length(arg_array); i++){
			arg = (struct argBuffer*)array_get(arg_array, i);
			free(arg->data);
		}

		array_delete(arg_array);
	}
}