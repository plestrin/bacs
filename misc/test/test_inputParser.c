#include <stdlib.h>
#include <stdio.h>

#include "../inputParser.h"

void print(uint32_t* ctx, char* string);
void print_hello(uint32_t* ctx);

int main(int argc, char** argv){
	struct inputParser 	parser;
	uint32_t 			print_context = 0;

	if (inputParser_init(&parser)){
		printf("ERROR: in %s, unable to init inputTracer\n", __func__);
		return 0;
	}

	if (inputParser_add_cmd(&parser, "print", "Print a message", "Message to be printed", INPUTPARSER_CMD_TYPE_ARG, &print_context, (void(*)(void))print)){
		printf("ERROR: in %s, unable to add cmd to inputParser\n", __func__);
	}
	if (!inputParser_add_cmd(&parser, "helloworld", "Print \"Hello World\"", NULL, INPUTPARSER_CMD_TYPE_NO_ARG, &print_context, (void(*)(void))print_hello)){
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

void print_hello(uint32_t* ctx){
	printf("%u - Hello World\n", *ctx);
	(*ctx) ++;
}