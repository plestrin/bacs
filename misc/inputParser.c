#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "inputParser.h"
#include "multiColumn.h"
#include "base.h"

#define INPUTPARSER_LINE_SIZE 512 /* must be at least larger or equal than INPUTPARSER_NAME_SIZE */

static int32_t inputParser_search_cmd(struct inputParser* parser, const char* cmd);
#ifdef INTERACTIVE
static uint32_t inputParser_complete_cmd(char* buffer, uint32_t buffer_length, uint32_t offset, struct inputParser* parser);
static int32_t inputParser_hist_up(char* buffer, struct inputParser* parser);
static int32_t inputParser_hist_down(char* buffer, struct inputParser* parser);
#endif

static void inputParser_print_help(struct inputParser* parser, const char* arg);
static void inputParser_exit(struct inputParser* parser);

static char* inputParser_get_argument(const char* cmd, char* line);

struct inputParser* inputParser_create(void){
	struct inputParser* parser;

	parser = malloc(sizeof(struct inputParser));
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
		if (inputParser_add_cmd(parser, "help", "Display this help", "str", INPUTPARSER_CMD_TYPE_OPT_ARG, parser, (void(*)(void))inputParser_print_help)){
			log_warn("unable to add help entry in the input parser");
		}
		if (inputParser_add_cmd(parser, "exit", "Exit", NULL, INPUTPARSER_CMD_TYPE_NO_ARG, parser, (void(*)(void))inputParser_exit)){
			log_warn("unable to add exit entry in the input parser");
		}

		#ifdef INTERACTIVE
		termReader_init(&(parser->term));
		if (termReader_set_raw_mode(&(parser->term))){
			log_err("unable to set terminal raw mode");
		}
		termReader_set_tab_handler(&(parser->term), inputParser_complete_cmd, parser)

		if ((parser->hist_array = array_create(INPUTPARSER_LINE_SIZE)) == NULL){
			log_err("unable to create array");
		}
		else{
			parser->hist_index = 0;
			termReader_set_history_handler(&(parser->term), inputParser_hist_up, inputParser_hist_down, parser)
		}
		#endif
	}

	return 0;
}

int inputParser_add_cmd(struct inputParser* parser, char* name, char* cmd_desc, char* arg_desc, enum cmdEntryType type, void* ctx, void(*func)(void)){
	struct cmdEntry entry;
	int 			result = -1;
	int32_t 		duplicate;

	if (name != NULL && cmd_desc != NULL && (arg_desc != NULL || type == INPUTPARSER_CMD_TYPE_NO_ARG)){
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
	char* 				sep;
	char* 				cmd;

	for (parser->exit = 0; !parser->exit; ){
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
				#ifdef COLOR
				printf(ANSI_COLOR_RESET);
				fflush(stdout);
				#endif
				return;
			}
			#else
			if (fgets(line, INPUTPARSER_LINE_SIZE, stdin) == NULL){
				#ifdef COLOR
				printf(ANSI_COLOR_RESET);
				fflush(stdout);
				#endif
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

		#ifdef INTERACTIVE
		if (parser->hist_array != NULL){
			if ((parser->hist_index = array_add(parser->hist_array, line)) < 0){
				log_err("unable to add element to array");
				parser->hist_index = array_get_length(parser->hist_array);
			}
			else{
				parser->hist_index ++;
			}
		}
		#endif

		for (cmd = line, sep = line; sep != NULL; cmd = sep + 1){
			while (*cmd == ' '){
				cmd++;
			}

			if ((sep = strchr(line, ';')) != NULL){
				*sep = '\0';
			}

			if ((entry_index = inputParser_search_cmd(parser, cmd)) >= 0){
				entry = (struct cmdEntry*)array_get(&(parser->cmd_array), (uint32_t)entry_index);
				if (entry->type == INPUTPARSER_CMD_TYPE_OPT_ARG){
					((void(*)(void*,char*))(entry->function))(entry->context, inputParser_get_argument(entry->name, cmd));
				}
				else if (entry->type == INPUTPARSER_CMD_TYPE_ARG){
					if ((cmd_arg = inputParser_get_argument(entry->name, cmd)) == NULL){
						log_err_m("this command requires at least one additional argument: %s", entry->arg_desc);
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
				printf("The syntax of the command is incorrect: \"%s\" (length: %u)\n", cmd, strlen(cmd));
			}
		}
	}
}

void inputParser_clean(struct inputParser* parser){
	array_clean(&(parser->cmd_array));

	#ifdef INTERACTIVE
	if (termReader_reset_mode(&(parser->term))){
		log_err("unable to reset terminal mode");
	}

	if (parser->hist_array != NULL){
		array_delete(parser->hist_array);
	}
	#endif
}

static int32_t inputParser_search_cmd(struct inputParser* parser, const char* cmd){
	uint32_t 			i;
	int32_t 			compare_result;
	struct cmdEntry* 	entry;

	for (i = 0; i < array_get_length(&(parser->cmd_array)); i++){
		entry = array_get(&(parser->cmd_array), i);
		if (entry->type == INPUTPARSER_CMD_TYPE_NO_ARG){
			compare_result = strncmp(entry->name, cmd, INPUTPARSER_NAME_SIZE);
		}
		else{
			compare_result = strncmp(entry->name, cmd, strlen(entry->name));
		}

		if (!compare_result){
			return (int32_t)i;
		}
	}

	return -1;
}

#ifdef INTERACTIVE

static uint32_t inputParser_complete_cmd(char* buffer, uint32_t buffer_length, uint32_t offset, struct inputParser* parser){
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

static int32_t inputParser_hist_up(char* buffer, struct inputParser* parser){
	if (parser->hist_index){
		parser->hist_index --;

		strncpy(buffer, array_get(parser->hist_array, parser->hist_index), INPUTPARSER_LINE_SIZE);
		return 1;
	}

	return 0;
}

static int32_t inputParser_hist_down(char* buffer, struct inputParser* parser){
	if ((uint32_t)parser->hist_index + 1 < array_get_length(parser->hist_array)){
		parser->hist_index ++;

		strncpy(buffer, array_get(parser->hist_array, parser->hist_index), INPUTPARSER_LINE_SIZE);
	}
	else if ((uint32_t)parser->hist_index + 1 == array_get_length(parser->hist_array)){
		parser->hist_index ++;
		buffer[0] = '\0';
	}
	else{
		buffer[0] = '\0';
	}

	return 1;
}

#endif

static void inputParser_print_help(struct inputParser* parser, const char* arg){
	uint32_t 					i;
	struct multiColumnPrinter* 	printer;
	struct cmdEntry* 			entry;
	size_t 						dyn_col_size_name;
	size_t 						dyn_col_size_desc;
	size_t 						local_size;

	printer = multiColumnPrinter_create(stdout, 4, NULL, NULL, NULL);
	if (printer == NULL){
		log_err("unable to create multiColumnPrinter");
		return;
	}

	multiColumnPrinter_set_title(printer, 0, "CMD");
	multiColumnPrinter_set_title(printer, 1, "ARG");
	multiColumnPrinter_set_title(printer, 2, "CMD DESC");
	multiColumnPrinter_set_title(printer, 3, "ARG DESC");

	for (i = 0, dyn_col_size_name = 0, dyn_col_size_desc = 0; i < array_get_length(&(parser->cmd_array)); i++){
		entry = (struct cmdEntry*)array_get(&(parser->cmd_array), i);

		local_size = strlen(entry->name);
		if (local_size > dyn_col_size_name){
			dyn_col_size_name = local_size;
		}

		local_size = strlen(entry->cmd_desc);
		if (local_size > dyn_col_size_desc){
			dyn_col_size_desc = local_size;
		}
	}

	multiColumnPrinter_set_column_size(printer, 0, (dyn_col_size_name > 24) ? 24 : dyn_col_size_name);
	multiColumnPrinter_set_column_size(printer, 1, 3);
	multiColumnPrinter_set_column_size(printer, 2, (dyn_col_size_desc > 48) ? 48 : dyn_col_size_desc);


	multiColumnPrinter_set_column_type(printer, 3, MULTICOLUMN_TYPE_UNBOUND_STRING);

	puts("List of available command(s):");

	multiColumnPrinter_print_header(printer);

	for (i = 0; i < array_get_length(&(parser->cmd_array)); i++){
		entry = (struct cmdEntry*)array_get(&(parser->cmd_array), i);

		if (arg != NULL && strstr(entry->name, arg) == NULL){
			continue;
		}

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

static char* inputParser_get_argument(const char* cmd, char* line){
	char* result;

	result = line + strlen(cmd);
	while (*result == ' '){
		result++;
	}

	if (*result == '\0'){
		result = NULL;
	}

	return result;
}

void inputParser_extract_range(const char* input, uint32_t* start, uint32_t* stop){
	enum rangeParserState{
		RANGE_PARSER_IDLE,
		RANGE_PARSER_INDEX1,
		RANGE_PARSER_NEG_INDEX1,
		RANGE_PARSER_SEP,
		RANGE_PARSER_INDEX2,
		RANGE_PARSER_NEG_INDEX2,
		RANGE_PARSER_END
	};

	size_t 					i;
	enum rangeParserState 	state;
	uint32_t 				tmp;

	if (input == NULL){
		return;
	}

	for (i = 0, state = RANGE_PARSER_IDLE; input[i] != '\0' && state != RANGE_PARSER_END; i++){
		switch (input[i]){
			case '-' : {
				if (state == RANGE_PARSER_INDEX1){
					state = RANGE_PARSER_NEG_INDEX1;
				}
				else if (state == RANGE_PARSER_INDEX2){
					state = RANGE_PARSER_NEG_INDEX2;
				}
				else{
					state = RANGE_PARSER_IDLE;
				}
				break;
			}
			case '0' :
			case '1' :
			case '2' :
			case '3' :
			case '4' :
			case '5' :
			case '6' :
			case '7' :
			case '8' :
			case '9' : {
				if (state == RANGE_PARSER_INDEX1){
					*start = atoi(input + i);
					state = RANGE_PARSER_SEP;
				}
				else if (state == RANGE_PARSER_NEG_INDEX1){
					tmp = atoi(input + i);
					if (tmp < *stop){
						*start = *stop - tmp;
					}
					state = RANGE_PARSER_SEP;
				}
				else if (state == RANGE_PARSER_INDEX2){
					*stop = atoi(input + i);
					state = RANGE_PARSER_END;
				}
				else if (state == RANGE_PARSER_NEG_INDEX2){
					tmp = atoi(input + i);
					if (tmp < *stop){
						*stop = *stop - tmp;
					}
					state = RANGE_PARSER_END;
				}
				else if (state != RANGE_PARSER_SEP){
					state = RANGE_PARSER_IDLE;
				}
				break;
			}
			case ':' : {
				if (state == RANGE_PARSER_SEP || state == RANGE_PARSER_INDEX1){
					state = RANGE_PARSER_INDEX2;
				}
				else{
					state = RANGE_PARSER_IDLE;
				}
				break;
			}
			case '[' : {
				if (state == RANGE_PARSER_IDLE){
					state = RANGE_PARSER_INDEX1;
				}
				else{
					state = RANGE_PARSER_IDLE;
				}
				break;
			}
			case ']' : {
				if (state == RANGE_PARSER_INDEX2){
					state = RANGE_PARSER_END;
				}
				else{
					state = RANGE_PARSER_IDLE;
				}
				break;
			}
			default : {
				state = RANGE_PARSER_IDLE;
			}
		}
	}

	if (state == RANGE_PARSER_END || state == RANGE_PARSER_INDEX2){
		return;
	}

	for (i = 0, state = RANGE_PARSER_INDEX1; input[i] != '\0'; i++){
		if (input[i] == ' '){
			state = RANGE_PARSER_INDEX1;
		}
		else if (input[i] >= '0' && input[i] <= '9'){
			if (state == RANGE_PARSER_INDEX1){
				*start = atoi(input + i);
				*stop = *start + 1;
				break;
			}
		}
		else{
			state = RANGE_PARSER_IDLE;
		}
	}
}
