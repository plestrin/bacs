#ifndef INPUTPARSER_H
#define INPUTPARSER_H

#include "array.h"
#ifdef INTERACTIVE
#include "termReader.h"
#endif

#define INPUTPARSER_NAME_SIZE 64
#define INPUTPARSER_DESC_SIZE 256

enum cmdEntryType{
	INPUTPARSER_CMD_TYPE_NO_ARG 	= 0,
	INPUTPARSER_CMD_TYPE_OPT_ARG 	= 1,
	INPUTPARSER_CMD_TYPE_ARG 		= 2
};

struct cmdEntry{
	char 				name[INPUTPARSER_NAME_SIZE];
	char 				cmd_desc[INPUTPARSER_DESC_SIZE];
	char 				arg_desc[INPUTPARSER_DESC_SIZE];
	enum cmdEntryType 	type;
	void* 				context;
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
	struct array* 		hist_array;
	int32_t 			hist_index;
#endif
	int32_t 			exit;
};

struct inputParser* inputParser_create(void);
int inputParser_init(struct inputParser* parser);
int inputParser_add_cmd(struct inputParser* parser, char* name, char* cmd_desc, char* arg_desc, enum cmdEntryType type, void* ctx, void(*func)(void));
void inputParser_exe(struct inputParser* parser, uint32_t argc, char** argv);
void inputParser_clean(struct inputParser* parser);

#define inputParser_delete(parser) 				\
	inputParser_clean(parser); 					\
	free(parser);

void inputParser_extract_range(const char* input, uint32_t* start, uint32_t* stop);

#endif
