#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "inputParser.h"
#include "multiColumn.h"
#include "base.h"

int32_t inputParser_search_cmd(struct inputParser* parser, char* cmd);
#ifdef INTERACTIVE
uint32_t inputParser_complete_cmd(char* buffer, uint32_t buffer_length, uint32_t offset, struct inputParser* parser);
#endif

static void inputParser_print_help(struct inputParser* parser);
static void inputParser_exit(struct inputParser* parser);

static char* inputParser_get_argument(char* cmd, char* line);

struct inputParser* inputParser_create(void){
	struct inputParser* parser;

	parser = (struct inputParser*)malloc(sizeof(struct inputParser));
	if (parser != NULL){
		if (inputParser_init(parser)){
			free(parser);
			parser = NULL;
		}
	}
	else{
		log_err("unable to allocate memory");
	}
	
	return parser;	
}

int inputParser_init(struct inputParser* parser){
	if (array_init(&(parser->cmd_array), sizeof(struct cmdEntry))){
		log_err("unable to init array");
		return -1;
	}
	else{
		if (inputParser_add_cmd(parser, "help", "Display this help", NULL, INPUTPARSER_CMD_TYPE_NO_ARG, parser,  (void(*)(void))inputParser_print_help)){
			log_warn("unable to add help entry in the input parser");
		}
		if (inputParser_add_cmd(parser, "exit", "Exit", NULL, INPUTPARSER_CMD_TYPE_NO_ARG, parser, (void(*)(void))inputParser_exit)){
			log_warn("unable to add exit entry in the input parser");
		}

		#ifdef INTERACTIVE
		if(termReader_set_raw_mode(&(parser->term))){
			log_err("unable to set terminal raw mode");
		}
		termReader_set_tab_handler(&(parser->term), inputParser_complete_cmd, parser);
		#endif
	}

	return 0;
}

int inputParser_add_cmd(struct inputParser* parser, char* name, char* cmd_desc, char* arg_desc, enum cmdEntryType type, void* ctx, void(*func)(void)){
	struct cmdEntry entry;
	int 			result = -1;
	int32_t 		duplicate;

	if (name != NULL && cmd_desc != NULL  && (arg_desc != NULL || type == INPUTPARSER_CMD_TYPE_NO_ARG)){
		duplicate = inputParser_search_cmd(parser, name);
		if (duplicate >= 0){
			log_err_m("\"%s\" is already registered as a command", name);
			return result;
		}

		if (strlen(name) > INPUTPARSER_NAME_SIZE){
			log_warn("name length is larger than INPUTPARSER_NAME_SIZE");
		}

		if (strlen(cmd_desc) > INPUTPARSER_DESC_SIZE){
			log_warn("cmd_desc length is larger than INPUTPARSER_DESC_SIZE");
		}

		if (arg_desc != NULL && strlen(arg_desc) > INPUTPARSER_DESC_SIZE){
			log_warn("arg_desc length is larger than INPUTPARSER_DESC_SIZE");
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
			log_warn("interactive mode is incorrect, setting to default not interactive mode");
			type = INPUTPARSER_CMD_TYPE_NO_ARG;
		}

		entry.type 		= type;
		entry.context 	= ctx;
		entry.function 	= func;

		result = (array_add(&(parser->cmd_array), &entry) < 0);
		if (result){
			log_err("unable to add element to array");
		}
	}
	else{
		log_err("some mandatory argument(s) are NULL");
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
			printf(ANSI_COLOR_CYAN ">>> %s" ANSI_COLOR_RESET "\n", line);
		}
		else{
			printf(ANSI_COLOR_CYAN ">>> ");
			fflush(stdout);

			#ifdef INTERACTIVE
			if (termReader_get_line(&(parser->term), line, INPUTPARSER_LINE_SIZE)){
				log_err("EOF");
				return;
			}
			#else
			if (fgets(line, INPUTPARSER_LINE_SIZE, stdin) == NULL){
				log_err("EOF");
				return;
			}
			else{
				line[strlen(line) - 1] = '\0';
			}
			#endif

			#ifdef COLOR
			printf(ANSI_COLOR_RESET);
			fflush(stdout);
			#endif
		}

		if ((entry_index = inputParser_search_cmd(parser, line)) >= 0){
			entry = (struct cmdEntry*)array_get(&(parser->cmd_array), (uint32_t)entry_index);
			if (entry->type == INPUTPARSER_CMD_TYPE_OPT_ARG){
				((void(*)(void*,char*))(entry->function))(entry->context, inputParser_get_argument(entry->name, line));
			}
			else if (entry->type == INPUTPARSER_CMD_TYPE_ARG){
				if ((cmd_arg = inputParser_get_argument(entry->name, line)) == NULL){
					log_err_m("this command requires at least one additionnal argument: %s", entry->arg_desc);
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
		log_err("unable to reset terminal mode");
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
			return (int32_t)i;
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
		log_err("unable to create multiColumnPrinter");
		return;
	}

	multiColumnPrinter_set_title(printer, 0, "CMD");
	multiColumnPrinter_set_title(printer, 1, "ARG");
	multiColumnPrinter_set_title(printer, 2, "CMD DESC");
	multiColumnPrinter_set_title(printer, 3, "ARG DESC");

	multiColumnPrinter_set_column_size(printer, 0, 24);
	multiColumnPrinter_set_column_size(printer, 1, 3);
	multiColumnPrinter_set_column_size(printer, 2, 48);
	multiColumnPrinter_set_column_size(printer, 3, 32);

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
					*start = (uint32_t)atoi(offset1 + 1);
					*stop  = (uint32_t)atoi(offset2 + 1);
				}
				else{
					*start = (uint32_t)atoi(offset1 + 1);
					*stop  = *start + 1;
				}
			}
			else{
				*start = (uint32_t)atoi(offset1);
				*stop  = *start + 1;
			}
		}
	}
}