#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "inputParser.h"
#include "multiColumn.h"

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

int32_t inputParser_search_cmd(struct cmdEntry* entry, char* cmd);

static void inputParser_print_help(struct inputParser* parser);
static void inputParser_exit(struct inputParser* parser);

static char* inputParser_get_argument(char* cmd, char* line);

struct inputParser* inputParser_create(){
	struct inputParser* parser;

	parser = (struct inputParser*)malloc(sizeof(struct inputParser));
	if (parser != NULL){
		if (inputParser_init(parser)){
			free(parser);
			parser = NULL;
		}
	}
	else{
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
	}
	
	return parser;	
}

int inputParser_init(struct inputParser* parser){
	int result = -1;

	if (parser != NULL){
		result = array_init(&(parser->cmd_array), sizeof(struct cmdEntry));
		if (result){
			printf("ERROR: in %s, unable to init array\n", __func__);
		}
		else{
			if (inputParser_add_cmd(parser, "help", "Display this help", INPUTPARSER_CMD_NOT_INTERACTIVE, parser,  (void(*)(void))inputParser_print_help)){
				printf("WARNING: in %s, unable to add help entry in the input parser\n", __func__);
			}
			if (inputParser_add_cmd(parser, "exit", "Exit", INPUTPARSER_CMD_NOT_INTERACTIVE, parser, (void(*)(void))inputParser_exit)){
				printf("WARNING: in %s, unable to add exit entry in the input parser\n", __func__);
			}
		}
	}

	return result;
}

int inputParser_add_cmd(struct inputParser* parser, char* name, char* desc, char interactive, void* ctx, void(*func)(void)){
	struct cmdEntry entry;
	int 			result = -1;
	int32_t 		duplicate;

	if (parser != NULL){
		duplicate = array_search_seq_up(&(parser->cmd_array), 0, array_get_length(&(parser->cmd_array)), name, (int32_t(*)(void*, void*))inputParser_search_cmd);
		if (duplicate >= 0){
			printf("ERROR: in %s, \"%s\" is already registered as a command\n", __func__, name);
			return result;
		}

		if (strlen(name) > INPUTPARSER_NAME_SIZE){
			printf("WARNING: in %s, name length is larger than INPUTPARSER_NAME_SIZE\n", __func__);
		}

		if (strlen(desc) > INPUTPARSER_DESC_SIZE){
			printf("WARNING: in %s, desc length is larger than INPUTPARSER_DESC_SIZE\n", __func__);
		}

		strncpy(entry.name, name, INPUTPARSER_NAME_SIZE);
		strncpy(entry.description, desc, INPUTPARSER_DESC_SIZE);

		if (interactive != INPUTPARSER_CMD_INTERACTIVE && interactive != INPUTPARSER_CMD_NOT_INTERACTIVE){
			printf("WARNING: in %s, interactive mode is incorrect, setting to default not interactive mode\n", __func__);
			interactive = INPUTPARSER_CMD_NOT_INTERACTIVE;
		}

		entry.interactive = interactive;
		entry.context = ctx;
		entry.function = func;

		result = (array_add(&(parser->cmd_array), &entry) < 0);
		if (result){
			printf("ERROR: in %s, unable to add element to array\n", __func__);
		}
	}

	return result;
}

#include "printBuffer.h" /* pour le debug */ 

void inputParser_exe(struct inputParser* parser, uint32_t argc, char** argv){
	uint32_t 			i;
	int32_t 			entry_index;
	struct cmdEntry* 	entry;
	char 				line[INPUTPARSER_LINE_SIZE];
	char 				character;
	uint32_t 			cmd_counter = 0;

	if (parser != NULL){

		parser->exit = 0;
		while(!parser->exit){
			printf(ANSI_COLOR_CYAN ">>> ");
			fflush(stdout);

			if (cmd_counter < argc){
				strncpy(line, argv[cmd_counter], INPUTPARSER_LINE_SIZE);
				printf("%s\n", line);
				cmd_counter ++;
			}
			else{
				i = 0;
				do{
					character = fgetc(stdin);
					switch(character){
					case '\t' 	: {break;}
					case '\n' 	: {break;}
					case EOF	: {printf("ERROR: in %s, fgetc returns EOF on stdin\n", __func__); return;}
					default 	: {line[i++] = character; break;}
					}
				} while(character != '\n' && i < INPUTPARSER_LINE_SIZE - 1);
				line[i] = '\0';
			}

			printf(ANSI_COLOR_RESET);

			entry_index = array_search_seq_up(&(parser->cmd_array), 0, array_get_length(&(parser->cmd_array)), line, (int32_t(*)(void*, void*))inputParser_search_cmd);
			if (entry_index >= 0){
				entry = (struct cmdEntry*)array_get(&(parser->cmd_array), entry_index);
				if (entry->interactive == INPUTPARSER_CMD_INTERACTIVE){
					((void(*)(void*,char*))(entry->function))(entry->context, inputParser_get_argument(entry->name, line));
				}
				else{
					((void(*)(void*))(entry->function))(entry->context);
				}
			}
			else{
				printf("The syntax of the command is incorrect: \"%s\" (length: %u)\n", line, strlen(line));
				inputParser_print_help(parser);
			}
		}
	}
}

void inputParser_clean(struct inputParser* parser){
	if (parser != NULL){
		array_clean(&(parser->cmd_array));
	}
}

void inputParser_delete(struct inputParser* parser){
	if (parser != NULL){
		inputParser_clean(parser);
		free(parser);
	}
}

int32_t inputParser_search_cmd(struct cmdEntry* entry, char* cmd){
	if (entry->interactive == INPUTPARSER_CMD_NOT_INTERACTIVE){
		return strncmp(entry->name, cmd, INPUTPARSER_NAME_SIZE);
	}
	else{
		return strncmp(entry->name, cmd, strlen(entry->name));
	}
}

static void inputParser_print_help(struct inputParser* parser){
	uint32_t 					i;
	struct multiColumnPrinter* 	printer;
	struct cmdEntry* 			entry;

	printer = multiColumnPrinter_create(stdout, 3, NULL, NULL, " ");
	if (printer == NULL){
		printf("ERROR: in %s, unable to create multiColumnPrinter\n", __func__);
		return;
	}

	multiColumnPrinter_set_column_size(printer, 0, 30);
	multiColumnPrinter_set_column_size(printer, 1, 3);
	multiColumnPrinter_set_column_size(printer, 2, 96);

	printf("List of available command(s):\n");

	for (i = 0; i < array_get_length(&(parser->cmd_array)); i++){
		entry = (struct cmdEntry*)array_get(&(parser->cmd_array), i);
		if (entry->interactive == INPUTPARSER_CMD_INTERACTIVE){
			multiColumnPrinter_print(printer, entry->name, "ARG", entry->description);
		}
		else{
			multiColumnPrinter_print(printer, entry->name, "", entry->description);
		}
	}

	multiColumnPrinter_delete(printer);
}

static void inputParser_exit(struct inputParser* parser){
	parser->exit = 1;
}

static char* inputParser_get_argument(char* cmd, char* line){
	char* result;

	result = line + strlen(cmd);
	while(*result == ' '){
		result++;
	}

	if (*result == '\0'){
		result = NULL;
	}

	return result;
}