#ifndef INPUTPARSER_H
#define INPUTPARSER_H

#include "array.h"

#define INPUTPARSER_NAME_SIZE 64
#define INPUTPARSER_DESC_SIZE 256
#define INPUTPARSER_LINE_SIZE 512

struct cmdEntry{
	char 	name[INPUTPARSER_NAME_SIZE];
	char 	description[INPUTPARSER_DESC_SIZE];
	void* 	context;
	void(*function)(void* context);
};

struct inputParser{
	struct array 	cmd_array;
	char 			exit;
};

struct inputParser* inputParser_create();
int inputParser_init(struct inputParser* parser);
int inputParser_add_cmd(struct inputParser* parser, char* name, char* desc, void* ctx, void(*func)(void* ctx));
void inputParser_exe(struct inputParser* parser);
void inputParser_clean(struct inputParser* parser);
void inputParser_delete(struct inputParser* parser);

#endif