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

int32_t inputParser_search_cmd(struct inputParser* parser, char* cmd);
#ifdef INTERACTIVE
uint32_t inputParser_complete_cmd(char* buffer, uint32_t buffer_length, uint32_t offset, struct inputParser* parser);
#endif

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
	if (array_init(&(parser->cmd_array), sizeof(struct cmdEntry))){
		printf("ERROR: in %s, unable to init array\n", __func__);
		return -1;
	}
	else{
		if (inputParser_add_cmd(parser, "help", "Display this help", NULL, INPUTPARSER_CMD_TYPE_NO_ARG, parser,  (void(*)(void))inputParser_print_help)){
			printf("WARNING: in %s, unable to add help entry in the input parser\n", __func__);
		}
		if (inputParser_add_cmd(parser, "exit", "Exit", NULL, INPUTPARSER_CMD_TYPE_NO_ARG, parser, (void(*)(void))inputParser_exit)){
			printf("WARNING: in %s, unable to add exit entry in the input parser\n", __func__);
		}

		#ifdef INTERACTIVE
		if(termReader_set_raw_mode(&(parser->term))){
			printf("ERROR: in %s, unable to set terminal raw mode\n", __func__);
		}
		termReader_set_tab_handler(&(parser->term), inputParser_complete_cmd, parser);
		#endif
	}

	return 0;
}

int inputParser_add_cmd(struct inputParser* parser, char* name, char* cmd_desc, char* arg_desc, char type, void* ctx, void(*func)(void)){
	struct cmdEntry entry;
	int 			result = -1;
	int32_t 		duplicate;

	if (name != NULL && cmd_desc != NULL  && (arg_desc != NULL || type == INPUTPARSER_CMD_TYPE_NO_ARG)){
		duplicate = inputParser_search_cmd(parser, name);
		if (duplicate >= 0){
			printf("ERROR: in %s, \"%s\" is already registered as a command\n", __func__, name);
			return result;
		}

		if (strlen(name) > INPUTPARSER_NAME_SIZE){
			printf("WARNING: in %s, name length is larger than INPUTPARSER_NAME_SIZE\n", __func__);
		}

		if (strlen(cmd_desc) > INPUTPARSER_DESC_SIZE){
			printf("WARNING: in %s, cmd_desc length is larger than INPUTPARSER_DESC_SIZE\n", __func__);
		}

		if (arg_desc != NULL && strlen(arg_desc) > INPUTPARSER_DESC_SIZE){
			printf("WARNING: in %s, arg_desc length is larger than INPUTPARSER_DESC_SIZE\n", __func__);
		}

		strncpy(entry.name, name, INPUTPARSER_NAME_SIZE);
		strncpy(entry.cmd_desc, cmd_desc, INPUTPARSER_DESC_SIZE);
		if (arg_desc != NULL){
			strncpy(entry.arg_desc, arg_desc, INPUTPARSER_DESC_SIZE);
		}
		else{
			entry.arg_desc[0] = '\0';
		}

		if (type != INPUTPARSER_CMD_TYPE_NO_ARG && type != INPUTPARSER_CMD_TYPE_OPT_ARG && type != INPUTPARSER_CMD_TYPE_ARG){
			printf("WARNING: in %s, interactive mode is incorrect, setting to default not interactive mode\n", __func__);
			type = INPUTPARSER_CMD_TYPE_NO_ARG;
		}

		entry.type 		= type;
		entry.context 	= ctx;
		entry.function 	= func;

		result = (array_add(&(parser->cmd_array), &entry) < 0);
		if (result){
			printf("ERROR: in %s, unable to add element to array\n", __func__);
		}
	}
	else{
		printf("ERROR: in %s, some mandatory argument(s) are NULL\n", __func__);
	}

	return result;
}

void inputParser_exe(struct inputParser* parser, uint32_t argc, char** argv){
	int32_t 			entry_index;
	struct cmdEntry* 	entry;
	char 				line[INPUTPARSER_LINE_SIZE];
	uint32_t 			cmd_counter = 0;
	char* 				cmd_arg;

	parser->exit = 0;
	while(!parser->exit){
		if (cmd_counter < argc){
			strncpy(line, argv[cmd_counter], INPUTPARSER_LINE_SIZE);
			cmd_counter ++;
			#ifdef VERBOSE
			printf(ANSI_COLOR_CYAN ">>> %s" ANSI_COLOR_RESET "\n", line);
			#else
			printf(">>> %s\n", line);
			#endif
		}
		else{
			#ifdef VERBOSE
			printf(ANSI_COLOR_CYAN ">>> ");
			#else
			printf(">>> ");
			#endif
			fflush(stdout);

			#ifdef INTERACTIVE
			if (termReader_get_line(&(parser->term), line, INPUTPARSER_LINE_SIZE)){
				printf("ERROR: in %s, EOF\n", __func__);
				return;
			}
			#else
			if (fgets(line, INPUTPARSER_LINE_SIZE, stdin) == NULL){
				printf("ERROR: in %s, EOF\n", __func__);
				return;
			}
			else{
				line[strlen(line) - 1] = '\0';
			}
			#endif

			#ifdef VERBOSE
			printf(ANSI_COLOR_RESET);
			#endif
		}

		entry_index = inputParser_search_cmd(parser, line);
		if (entry_index >= 0){
			entry = (struct cmdEntry*)array_get(&(parser->cmd_array), entry_index);
			if (entry->type == INPUTPARSER_CMD_TYPE_OPT_ARG){
				((void(*)(void*,char*))(entry->function))(entry->context, inputParser_get_argument(entry->name, line));
			}
			else if (entry->type == INPUTPARSER_CMD_TYPE_ARG){
				cmd_arg = inputParser_get_argument(entry->name, line);
				if (cmd_arg == NULL){
					printf("ERROR: in %s, this command requires at least one additionnal argument: %s\n", __func__, entry->arg_desc);
				}
				else{
					((void(*)(void*,char*))(entry->function))(entry->context, cmd_arg);
				}
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

void inputParser_clean(struct inputParser* parser){
	array_clean(&(parser->cmd_array));

	#ifdef INTERACTIVE
	if(termReader_reset_mode(&(parser->term))){
		printf("ERROR: in %s, unable to reset terminal mode\n", __func__);
	}
	#endif
}

int32_t inputParser_search_cmd(struct inputParser* parser, char* cmd){
	uint32_t 			i;
	int32_t 			compare_result;
	struct cmdEntry* 	entry;

	for (i = 0; i < array_get_length(&(parser->cmd_array)); i++){
		entry = (struct cmdEntry*)array_get(&(parser->cmd_array), i);
		if (entry->type == INPUTPARSER_CMD_TYPE_NO_ARG){
			compare_result = strncmp(entry->name, cmd, INPUTPARSER_NAME_SIZE);
		}
		else{
			compare_result = strncmp(entry->name, cmd, strlen(entry->name));
		}

		if (compare_result == 0){
			return i;
		}
	}

	return -1;
}

#ifdef INTERACTIVE
uint32_t inputParser_complete_cmd(char* buffer, uint32_t buffer_length, uint32_t offset, struct inputParser* parser){
	uint32_t 			i;
	uint32_t 			j;
	struct cmdEntry* 	entry;
	uint32_t 			completion_offset = offset;
	uint32_t 			find_previous = 0;

	for (i = 0; i < array_get_length(&(parser->cmd_array)); i++){
		entry = (struct cmdEntry*)array_get(&(parser->cmd_array), i);
		if (!strncmp(entry->name, buffer, offset)){
			if (find_previous){
				for (j = offset; j < completion_offset; j++){
					if (buffer[j] != entry->name[j]){
						break;
					}
				}
				completion_offset = j;
			}
			else{
				strncpy(buffer + offset, entry->name + offset, ((INPUTPARSER_NAME_SIZE > buffer_length) ? (buffer_length - offset) : (INPUTPARSER_NAME_SIZE - offset)));
				completion_offset =	strlen(entry->name);
				find_previous = 1;
			}
		}
	}

	return completion_offset;
}
#endif

static void inputParser_print_help(struct inputParser* parser){
	uint32_t 					i;
	struct multiColumnPrinter* 	printer;
	struct cmdEntry* 			entry;

	printer = multiColumnPrinter_create(stdout, 4, NULL, NULL, NULL);
	if (printer == NULL){
		printf("ERROR: in %s, unable to create multiColumnPrinter\n", __func__);
		return;
	}

	multiColumnPrinter_set_title(printer, 0, "CMD");
	multiColumnPrinter_set_title(printer, 1, "ARG");
	multiColumnPrinter_set_title(printer, 2, "CMD DESC");
	multiColumnPrinter_set_title(printer, 3, "ARG DESC");

	multiColumnPrinter_set_column_size(printer, 0, 24);
	multiColumnPrinter_set_column_size(printer, 1, 3);
	multiColumnPrinter_set_column_size(printer, 2, 64);
	multiColumnPrinter_set_column_size(printer, 3, 48);

	printf("List of available command(s):\n");

	multiColumnPrinter_print_header(printer);

	for (i = 0; i < array_get_length(&(parser->cmd_array)); i++){
		entry = (struct cmdEntry*)array_get(&(parser->cmd_array), i);
		if (entry->type == INPUTPARSER_CMD_TYPE_ARG){
			multiColumnPrinter_print(printer, entry->name, "ARG", entry->cmd_desc, entry->arg_desc);
		}
		else if (entry->type == INPUTPARSER_CMD_TYPE_OPT_ARG){
			multiColumnPrinter_print(printer, entry->name, "OPT", entry->cmd_desc, entry->arg_desc);
		}
		else{
			multiColumnPrinter_print(printer, entry->name, "", entry->cmd_desc, "-");
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

void inputParser_extract_index(char* input, uint32_t* start, uint32_t* stop){
	uint32_t 	length;
	char* 		offset1;
	char* 		offset2;

	if (input != NULL){
		offset1 = strpbrk(input, "0123456789[");

		if (offset1 != NULL){
			length = strlen(offset1);
			if (length >= 4 && offset1[0] == '['){
				offset2 = strchr(offset1, ':');
				if (offset2 != NULL){
					*start = atoi(offset1 + 1);
					*stop  = atoi(offset2 + 1);
				}
				else{
					*start = atoi(offset1 + 1);
					*stop  = *start + 1;
				}
			}
			else{
				*start = atoi(offset1);
				*stop  = *start + 1;
			}
		}
	}
}