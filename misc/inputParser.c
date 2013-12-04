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

static void inputParser_print_help(struct inputParser* parser);
static void inputParser_exit(struct inputParser* parser);

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
			if (inputParser_add_cmd(parser, "help", "Display this help", parser, (void(*)(void*))inputParser_print_help)){
				printf("WARNING: in %s, unable to add help entry in the input parser\n", __func__);
			}
			if (inputParser_add_cmd(parser, "exit", "Exit", parser, (void(*)(void*))inputParser_exit)){
				printf("WARNING: in %s, unable to add exit entry in the input parser\n", __func__);
			}
		}
	}

	return result;
}

int inputParser_add_cmd(struct inputParser* parser, char* name, char* desc, void* ctx, void(*func)(void* ctx)){
	struct cmdEntry entry;
	int 			result = -1;

	if (parser != NULL){
		/* Need to check if the current name is taken or not */
		/* Make a static routine to search the input */

		if (strlen(name) > INPUTPARSER_NAME_SIZE){
			printf("WARNING: in %s, name length is larger than INPUTPARSER_NAME_SIZE\n", __func__);
		}

		if (strlen(desc) > INPUTPARSER_DESC_SIZE){
			printf("WARNING: in %s, desc length is larger than INPUTPARSER_DESC_SIZE\n", __func__);
		}

		strncpy(entry.name, name, INPUTPARSER_NAME_SIZE);
		strncpy(entry.description, desc, INPUTPARSER_DESC_SIZE);

		entry.context = ctx;
		entry.function = func;

		result = (array_add(&(parser->cmd_array), &entry) < 0);
		if (result){
			printf("ERROR: in %s, unable to add element to array\n", __func__);
		}
	}

	return result;
}

void inputParser_exe(struct inputParser* parser){
	uint32_t 			i;
	struct cmdEntry* 	entry;
	char 				valid_cmd;
	char 				line[INPUTPARSER_LINE_SIZE];
	char 				character;

	if (parser != NULL){

		parser->exit = 0;
		while(!parser->exit){
			printf(ANSI_COLOR_CYAN ">>> ");

			i = 0;
			do{
				character = fgetc(stdin);
				switch(character){
				case '\t' 	: {break;}
				case '\n' 	: {break;}
				default 	: {line[i++] = character; break;}
				}
			} while(character != '\n' && i < INPUTPARSER_LINE_SIZE - 1);
			line[i] = '\0';

			printf(ANSI_COLOR_RESET);

			for (i = 0, valid_cmd = 0; i < array_get_length(&(parser->cmd_array)); i++){
				entry = (struct cmdEntry*)array_get(&(parser->cmd_array), i);
				if (!strncmp(entry->name, line, INPUTPARSER_NAME_SIZE)){
					entry->function(entry->context);
					valid_cmd = 1;
					break;
				}
			}

			if (valid_cmd == 0){
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

static void inputParser_print_help(struct inputParser* parser){
	uint32_t 					i;
	struct multiColumnPrinter* 	printer;
	struct cmdEntry* 			entry;

	printer = multiColumnPrinter_create(stdout, 2, NULL, " ");
	if (printer == NULL){
		printf("ERROR: in %s, unable to create multiColumnPrinter\n", __func__);
		return;
	}

	multiColumnPrinter_set_column_size(printer, 0, 32);
	multiColumnPrinter_set_column_size(printer, 1, 128);

	printf("List of available command(s):\n");

	for (i = 0; i < array_get_length(&(parser->cmd_array)); i++){
		entry = (struct cmdEntry*)array_get(&(parser->cmd_array), i);
		multiColumnPrinter_print(printer, entry->name, entry->description);
	}

	multiColumnPrinter_delete(printer);
}

static void inputParser_exit(struct inputParser* parser){
	parser->exit = 1;
}