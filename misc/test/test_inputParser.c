#include <stdlib.h>
#include <stdio.h>

#include "../inputParser.h"

void print(uint32_t* ctx, char* string);

int main(int argc, char** argv){
	struct inputParser 	parser;
	uint32_t 			print_context = 0;

	if (inputParser_init(&parser)){
		printf("ERROR: in %s, unable to init inputTracer\n", __func__);
		return 0;
	}

	if (inputParser_add_cmd(&parser, "print", "Print message given as second argument", INPUTPARSER_CMD_INTERACTIVE, &print_context, (void(*)(void))print)){
		printf("ERROR: in %s, unable to add cmd to inputParser\n", __func__);
	}
	if (!inputParser_add_cmd(&parser, "helloworld", "Print \"Hello World\"", INPUTPARSER_CMD_NOT_INTERACTIVE, "Hello World", (void(*)(void))print)){
		printf("WARNING: in %s, we add two times the same cmd, the second'll not be reachable\n", __func__);
	}
	
	inputParser_exe(&parser, argc - 1, argv + 1);

	inputParser_clean(&parser);

	return 0;
}

void print(uint32_t* ctx, char* string){
	printf("%u - %s\n", *ctx, string);
	(*ctx) ++;
}