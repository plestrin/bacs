#ifndef INPUTPARSER_H
#define INPUTPARSER_H

#include "array.h"
#ifdef INTERACTIVE
#include "termReader.h"
#endif

#define INPUTPARSER_NAME_SIZE 64
#define INPUTPARSER_DESC_SIZE 256
#define INPUTPARSER_LINE_SIZE 512 /* must be at least larger or equal than INPUTPARSER_NAME_SIZE */

#define INPUTPARSER_CMD_TYPE_NO_ARG 		0
#define INPUTPARSER_CMD_TYPE_OPT_ARG		1
#define INPUTPARSER_CMD_TYPE_ARG			2

struct cmdEntry{
	char 	name[INPUTPARSER_NAME_SIZE];
	char 	cmd_desc[INPUTPARSER_DESC_SIZE];
	char 	arg_desc[INPUTPARSER_DESC_SIZE];
	char 	type;
	void* 	context;
	void(*function)(void);
};

 /* Function type is:
  * - void(*)(void* ctx) for non interactive cmd (NO_ARG)
  * - void(*)(void* ctx, char* arg) for interactive cmd (ARG & OPT_ARG)
  */

struct inputParser{
	struct array 		cmd_array;
#ifdef INTERACTIVE
	struct termReader 	term;
#endif
	char 				exit;
};

struct inputParser* inputParser_create();
int inputParser_init(struct inputParser* parser);
int inputParser_add_cmd(struct inputParser* parser, char* name, char* cmd_desc, char* arg_desc, char type, void* ctx, void(*func)(void));
void inputParser_exe(struct inputParser* parser, uint32_t argc, char** argv);
void inputParser_clean(struct inputParser* parser);

#define inputParser_delete(parser) 				\
	inputParser_clean(parser); 					\
	free(parser)

void inputParser_extract_index(char* input, uint32_t* start, uint32_t* stop);

#endif