#ifndef INPUTPARSER_H
#define INPUTPARSER_H

#include "array.h"
#include "termReader.h"

#define INPUTPARSER_NAME_SIZE 64
#define INPUTPARSER_DESC_SIZE 256
#define INPUTPARSER_LINE_SIZE 512 /* must be at least larger or equal than INPUTPARSER_NAME_SIZE */

#define INPUTPARSER_CMD_INTERACTIVE 		1
#define INPUTPARSER_CMD_NOT_INTERACTIVE		0

struct cmdEntry{
	char 	name[INPUTPARSER_NAME_SIZE];
	char 	description[INPUTPARSER_DESC_SIZE];
	char 	interactive;
	void* 	context;
	void(*function)(void);
};

 /* Function type is:
  * - void(*)(void* ctx) for non interactive cmd 
  * - void(*)(void* ctx, char* arg) for interactive cmd 
  */

struct inputParser{
	struct array 		cmd_array;
	struct termReader 	term;
	char 				exit;
};

struct inputParser* inputParser_create();
int inputParser_init(struct inputParser* parser);
int inputParser_add_cmd(struct inputParser* parser, char* name, char* desc, char interactive, void* ctx, void(*func)(void));
void inputParser_exe(struct inputParser* parser, uint32_t argc, char** argv);
void inputParser_clean(struct inputParser* parser);
void inputParser_delete(struct inputParser* parser);

void inputParser_extract_index(char* input, uint32_t* start, uint32_t* stop);

#endif