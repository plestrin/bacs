#ifdef __PIN__

#include "pin.H"

#ifdef _WIN32
#include "windowsComp.h"
#endif

#include "codeMap.h"

int codeMap_is_instruction_whiteListed(struct codeMap* cm, ADDRESS address){
	struct cm_image* 	image_cursor;
	struct cm_section* 	section_cursor;
	struct cm_routine* 	routine_cursor;

	if (cm == NULL){
		return CODEMAP_NOT_WHITELISTED;
	}

	for (image_cursor = cm->images; image_cursor != NULL; image_cursor = image_cursor->next){
		if (CODEMAP_IS_ADDRESS_IN_IMAGE(image_cursor, address)){
			if (image_cursor->white_listed == CODEMAP_WHITELISTED){
				return CODEMAP_WHITELISTED;
			}

			for (section_cursor = image_cursor->sections; section_cursor != NULL; section_cursor = section_cursor->next){
				if (CODEMAP_IS_ADDRESS_IN_SECTION(section_cursor, address)){
					for (routine_cursor = section_cursor->routines; routine_cursor != NULL; routine_cursor = routine_cursor->next){
						if (CODEMAP_IS_ADDRESS_IN_ROUTINE(routine_cursor, address)){
							return routine_cursor->white_listed;
						}
					}
					break;
				}
			}
			break;
		}
	}

	return CODEMAP_NOT_WHITELISTED;
}

#ifdef __linux__

int codeMap_add_vdso(struct codeMap* cm, char white_listed){
	FILE* 		maps_file;
	char 		file_name[256];
	char* 		line = NULL;
	size_t 		length = 0;
	ssize_t 	read;
	ADDRESS 	address_start = 0;
	ADDRESS 	address_stop = 0;

	snprintf(file_name, 256, "/proc/%u/maps", PIN_GetPid());
	maps_file = fopen(file_name, "r");
	if (maps_file == NULL){
		LOG("ERROR: unable to open file: ");
		LOG(file_name);
		LOG("\n");
		return -1;
	}

	while ((read = getline(&line, &length, maps_file)) != -1) {
		if (strstr(line, "[vdso]") != NULL && read > 20){
			address_start = strtoul((const char*)line, NULL, 16);
			address_stop = strtoul((const char*)(line + 9), NULL, 16);
			break;
		}
	}

	if (line != NULL){
		free(line);
	}

	if (address_start == 0 && address_stop == 0){
		LOG("ERROR: unable to locate VDSO\n");
		return -1;
	}

	if (codeMap_add_image(cm, address_start, address_stop, "VDSO", white_listed)){
		LOG("ERROR: unable to add VDSO image to code map structure\n");
	}
	else{
		if (codeMap_add_section(cm, address_start, address_stop, "VDSO")){
			LOG("ERROR: unable to add section to code map structure\n");
		}
		else{
			if (codeMap_add_routine(cm, address_start, address_stop, "VDSO", white_listed) == NULL){
				LOG("ERROR: unable to add routine to code map structure\n");
			}
		}
	}

	return 0;
}

#endif

static void codeMap_print_routine_JSON(struct cm_routine* routine, FILE* file);
static void codeMap_print_section_JSON(struct cm_section* section, FILE* file);
static void codeMap_print_image_JSON(struct cm_image* image, FILE* file);

void codeMap_print_JSON(struct codeMap* cm, FILE* file){
	struct cm_image* image;

	if (cm != NULL){
		fprintf(file, "{\"image\":[");

		image = cm->images;
		while(image != NULL){
			codeMap_print_image_JSON(image, file);
			image = image->next;
			if (image != NULL){
				fprintf(file, ",");
			}
			else{
				fprintf(file, "]}");
			}
		}
	}
}

static void codeMap_print_routine_JSON(struct cm_routine* routine, FILE* file){
	if (routine != NULL){
		if (routine->white_listed == CODEMAP_WHITELISTED){
			fprintf(file, "{\"start\":\"" PRINTF_ADDR_SHORT "\",\"stop\":\"" PRINTF_ADDR_SHORT "\",\"name\":\"%s\",\"whl\":true,\"exe\":%u}", routine->address_start, routine->address_stop, routine->name, routine->nb_execution);
		}
		else{
			fprintf(file, "{\"start\":\"" PRINTF_ADDR_SHORT "\",\"stop\":\"" PRINTF_ADDR_SHORT "\",\"name\":\"%s\",\"whl\":false,\"exe\":%u}", routine->address_start, routine->address_stop, routine->name, routine->nb_execution);
		}
	}
}

static void codeMap_print_section_JSON(struct cm_section* section, FILE* file){
	struct cm_routine* routine;

	if (section != NULL){
		fprintf(file, "{\"start\":\"" PRINTF_ADDR_SHORT "\",\"stop\":\"" PRINTF_ADDR_SHORT "\",\"name\":\"%s\",\"routine\":[", section->address_start, section->address_stop, section->name);

		routine = section->routines;
		while(routine != NULL){
			codeMap_print_routine_JSON(routine, file);
			routine = routine->next;
			if (routine != NULL){
				fprintf(file, ",");
			}
			else{
				fprintf(file, "]}");
			}
		}
	}
}

static void codeMap_print_image_JSON(struct cm_image* image, FILE* file){
	struct cm_section* section;

	if (image != NULL){
		if (image->white_listed == CODEMAP_WHITELISTED){
			#ifdef _WIN32
			fprintf(file, "{\"start\":\"" PRINTF_ADDR_SHORT "\",\"stop\":\"" PRINTF_ADDR_SHORT "\",\"name\":\"%s\",\"whl\":true,\"section\":[", image->address_start, image->address_stop, windowsComp_sanitize_path(image->name));
			#else
			fprintf(file, "{\"start\":\"" PRINTF_ADDR_SHORT "\",\"stop\":\"" PRINTF_ADDR_SHORT "\",\"name\":\"%s\",\"whl\":true,\"section\":[", image->address_start, image->address_stop, image->name);
			#endif
		}
		else{
			#ifdef _WIN32
			fprintf(file, "{\"start\":\"" PRINTF_ADDR_SHORT "\",\"stop\":\"" PRINTF_ADDR_SHORT "\",\"name\":\"%s\",\"whl\":false,\"section\":[", image->address_start, image->address_stop, windowsComp_sanitize_path(image->name));
			#else
			fprintf(file, "{\"start\":\"" PRINTF_ADDR_SHORT "\",\"stop\":\"" PRINTF_ADDR_SHORT "\",\"name\":\"%s\",\"whl\":false,\"section\":[", image->address_start, image->address_stop, image->name);
			#endif
		}

		section = image->sections;
		while(section != NULL){
			codeMap_print_section_JSON(section, file);
			section = section->next;
			if (section != NULL){
				fprintf(file, ",");
			}
		}
		fprintf(file, "]}");
	}
}

#else

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "codeMap.h"
#include "multiColumn.h"

static void codeMap_print_routine(struct multiColumnPrinter* printer, struct cm_routine* routine, int filter);
static void codeMap_print_section(struct multiColumnPrinter* printer, struct cm_section* section, int filter);
static void codeMap_print_image(struct multiColumnPrinter* printer, struct cm_image* image, int filter);

#define codeMap_filter_routine_executed(routine) ((routine)->nb_execution != 0)
static int codeMap_filter_section_executed(struct cm_section* section);
static int codeMap_filter_image_executed(struct cm_image* image);

#define codeMap_filter_routine_whitelisted(routine) ((routine)->white_listed == CODEMAP_WHITELISTED)
static int codeMap_filter_section_whitelisted(struct cm_section* section);
static int codeMap_filter_image_whitelisted(struct cm_image* image);

int codeMap_add_static_image(struct codeMap* cm, struct cm_image* image){
	struct cm_image* 	new_image;
	struct cm_image** 	cursor = &(cm->images);

	if (cm != NULL){
		new_image = (struct cm_image*)malloc(sizeof(struct cm_image));
		if (new_image == NULL){
			printf("ERROR: in %s, unable to allocate memory\n", __func__);
			return -1;
		}
		else{
			memcpy(new_image, image, sizeof(struct cm_image));
			new_image->sections = NULL;
			new_image->next = NULL;
			new_image->parent = cm;

			while(*cursor != NULL){
				cursor = &((*cursor)->next);
			}
			*cursor = new_image;

			cm->current_image = new_image;
			cm->current_section = NULL;
		}
	}

	return 0;
}

int codeMap_add_static_section(struct codeMap* cm, struct cm_section* section){
	struct cm_section* 	new_section;
	struct cm_section** cursor;

	if (cm != NULL){
		if (cm->current_image != NULL){
			new_section = (struct cm_section*)malloc(sizeof(struct cm_section));
			if (new_section == NULL){
				printf("ERROR: in %s, unable to allocate memory\n", __func__);
				return -1;
			}
			else{
				memcpy(new_section, section, sizeof(struct cm_section));
				new_section->routines = NULL;
				new_section->next = NULL;
				new_section->parent = cm->current_image;

				cursor = &(cm->current_image->sections);
				while(*cursor != NULL){
					cursor = &((*cursor)->next);
				}
				*cursor = new_section;

				cm->current_section = new_section;
			}
		}
		else{
			printf("ERROR: in %s, current image is NULL\n", __func__);
			return -1;
		}
	}

	return 0;
}

int codeMap_add_static_routine(struct codeMap* cm, struct cm_routine* routine){
	struct cm_routine*	new_routine = NULL;
	struct cm_routine**	cursor;

	if (cm != NULL){
		if (cm->current_section != NULL){
			new_routine = (struct cm_routine*)malloc(sizeof(struct cm_routine));
			if (new_routine == NULL){
				printf("ERROR: in %s, unable to allocate memory\n", __func__);
				return -1;
			}
			else{
				memcpy(new_routine, routine, sizeof(struct cm_routine));
				new_routine->next = NULL;
				new_routine->parent = cm->current_section;

				cursor = &(cm->current_section->routines);
				while(*cursor != NULL){
					cursor = &((*cursor)->next);
				}
				*cursor = new_routine;
			}
		}
		else{
			printf("ERROR: in %s, current section is NULL\n", __func__);
			return -1;
		}
	}

	return 0;
}

#if defined ARCH_32
#define codeMap_create_printer(printer) 																								\
	if (((printer) = multiColumnPrinter_create(stdout, 7, NULL, NULL, NULL)) == NULL){ 													\
		printf("ERROR: in %s, unable to create multi column printer\n", __func__); 														\
	} 																																	\
																																		\
	multiColumnPrinter_set_column_size((printer), 0, 38); 																				\
	multiColumnPrinter_set_column_size((printer), 1, 7); 																				\
	multiColumnPrinter_set_column_size((printer), 2, 48); 																				\
	multiColumnPrinter_set_column_size((printer), 3, 10); 																				\
	multiColumnPrinter_set_column_size((printer), 4, 10); 																				\
	multiColumnPrinter_set_column_size((printer), 5, 1); 																				\
	multiColumnPrinter_set_column_size((printer), 6, 1); 																				\
																																		\
	multiColumnPrinter_set_column_type((printer), 3, MULTICOLUMN_TYPE_HEX_32); 															\
	multiColumnPrinter_set_column_type((printer), 4, MULTICOLUMN_TYPE_HEX_32); 															\
	multiColumnPrinter_set_column_type((printer), 5, MULTICOLUMN_TYPE_BOOL); 															\
	multiColumnPrinter_set_column_type((printer), 6, MULTICOLUMN_TYPE_BOOL); 															\
																																		\
	multiColumnPrinter_set_title((printer), 0, (char*)"IMAGE"); 																		\
	multiColumnPrinter_set_title((printer), 1, (char*)"SECTION"); 																		\
	multiColumnPrinter_set_title((printer), 2, (char*)"ROUTINE"); 																		\
	multiColumnPrinter_set_title((printer), 3, (char*)"START @"); 																		\
	multiColumnPrinter_set_title((printer), 4, (char*)"STOP @"); 																		\
	multiColumnPrinter_set_title((printer), 5, (char*)"W"); 																			\
	multiColumnPrinter_set_title((printer), 6, (char*)"E"); 																			\
	multiColumnPrinter_print_header((printer));
#elif defined ARCH_64
#define codeMap_create_printer(printer) 																								\
	if (((printer) = multiColumnPrinter_create(stdout, 7, NULL, NULL, NULL)) == NULL){ 													\
		printf("ERROR: in %s, unable to create multi column printer\n", __func__); 														\
	} 																																	\
																																		\
	multiColumnPrinter_set_column_size((printer), 0, 38); 																				\
	multiColumnPrinter_set_column_size((printer), 1, 7); 																				\
	multiColumnPrinter_set_column_size((printer), 2, 48); 																				\
	multiColumnPrinter_set_column_size((printer), 3, 10); 																				\
	multiColumnPrinter_set_column_size((printer), 4, 10); 																				\
	multiColumnPrinter_set_column_size((printer), 5, 1); 																				\
	multiColumnPrinter_set_column_size((printer), 6, 1); 																				\
																																		\
	multiColumnPrinter_set_column_type((printer), 3, MULTICOLUMN_TYPE_HEX_64); 															\
	multiColumnPrinter_set_column_type((printer), 4, MULTICOLUMN_TYPE_HEX_64); 															\
	multiColumnPrinter_set_column_type((printer), 5, MULTICOLUMN_TYPE_BOOL); 															\
	multiColumnPrinter_set_column_type((printer), 6, MULTICOLUMN_TYPE_BOOL); 															\
																																		\
	multiColumnPrinter_set_title((printer), 0, (char*)"IMAGE"); 																		\
	multiColumnPrinter_set_title((printer), 1, (char*)"SECTION"); 																		\
	multiColumnPrinter_set_title((printer), 2, (char*)"ROUTINE"); 																		\
	multiColumnPrinter_set_title((printer), 3, (char*)"START @"); 																		\
	multiColumnPrinter_set_title((printer), 4, (char*)"STOP @"); 																		\
	multiColumnPrinter_set_title((printer), 5, (char*)"W"); 																			\
	multiColumnPrinter_set_title((printer), 6, (char*)"E"); 																			\
	multiColumnPrinter_print_header((printer));
#else
#error Please specify an architecture {ARCH_32 or ARCH_64}
#endif

void codeMap_print(struct codeMap* cm, const char* str_filter){
	struct cm_image* 			image;
	struct multiColumnPrinter* 	printer;
	int 						filter = 0;
	unsigned int 				i;

	#ifdef VERBOSE
	printf("Available filters ('%c' = whitelist, '%c' = executed)\n", CODEMAP_FILTER_WHITELIST_CMD, CODEMAP_FILTER_EXECUTED_CMD);
	#endif

	if (cm != NULL){
		if (str_filter != NULL){
			for (i = 0; i < strlen(str_filter); i++){
				switch(str_filter[i]){
				case CODEMAP_FILTER_WHITELIST_CMD 	: {filter |= CODEMAP_FILTER_WHITELIST; break;}
				case CODEMAP_FILTER_EXECUTED_CMD 	: {filter |= CODEMAP_FILTER_EXECUTED; break;}
				default 							: {printf("WARNING: in %s, unknown filter specifier '%c'\n", __func__, str_filter[i]); break;}
				}
			}
		}

		if (filter){
			printf("*** Code Map - filter: { ");
			if (filter & CODEMAP_FILTER_WHITELIST){
				printf("WHITELIST ");
			}
			if (filter & CODEMAP_FILTER_EXECUTED){
				printf("EXECUTED ");
			}
			printf("} ***\n");
		}

		codeMap_create_printer(printer)

		for (image = cm->images; image != NULL; image = image->next){
			codeMap_print_image(printer, image, filter);
		}

		multiColumnPrinter_delete(printer);
	}
}

static void codeMap_print_routine(struct multiColumnPrinter* printer, struct cm_routine* routine, int filter){
	if (filter & CODEMAP_FILTER_EXECUTED && !codeMap_filter_routine_executed(routine)){
		return;
	}
	if (filter & CODEMAP_FILTER_WHITELIST && !codeMap_filter_routine_whitelisted(routine)){
		return;
	}

	multiColumnPrinter_print(printer, routine->parent->parent->name, routine->parent->name, routine->name, routine->address_start, routine->address_stop, codeMap_filter_routine_whitelisted(routine), codeMap_filter_routine_executed(routine), NULL);
}

static void codeMap_print_section(struct multiColumnPrinter* printer, struct cm_section* section, int filter){
	struct cm_routine* routine;

	if (filter & CODEMAP_FILTER_EXECUTED && !codeMap_filter_section_executed(section)){
		return;
	}
	if (filter & CODEMAP_FILTER_WHITELIST && !codeMap_filter_section_whitelisted(section)){
		return;
	}

	for (routine = section->routines; routine != NULL; routine = routine->next){
		codeMap_print_routine(printer, routine, filter);
	}
}

static void codeMap_print_image(struct multiColumnPrinter* printer, struct cm_image* image, int filter){
	struct cm_section* section;

	if (filter & CODEMAP_FILTER_EXECUTED && !codeMap_filter_image_executed(image)){
		return;
	}
	if (filter & CODEMAP_FILTER_WHITELIST && !codeMap_filter_image_whitelisted(image)){
		return;
	}

	for (section = image->sections; section != NULL; section = section->next){
		codeMap_print_section(printer, section, filter);
	}

	multiColumnPrinter_print_horizontal_separator(printer);
}

void codeMap_clean_image(struct cm_image* image){
	image->address_start 	= 0;
	image->address_stop 	= 0;
	memset(image->name, '\0', CODEMAP_DEFAULT_NAME_SIZE);
	image->white_listed 	= CODEMAP_NOT_WHITELISTED;
	image->sections 		= NULL;
	image->next 			= NULL;
	image->parent 			= NULL;
}

void codeMap_clean_section(struct cm_section* section){
	section->address_start 	= 0;
	section->address_stop 	= 0;
	memset(section->name, '\0', CODEMAP_DEFAULT_NAME_SIZE);
	section->routines 		= NULL;
	section->next 			= NULL;
	section->parent 		= NULL;
}

void codeMap_clean_routine(struct cm_routine* routine){
	routine->address_start 	= 0;
	routine->address_stop 	= 0;
	memset(routine->name, '\0', CODEMAP_DEFAULT_NAME_SIZE);
	routine->nb_execution 	= 0;
	routine->white_listed 	= CODEMAP_NOT_WHITELISTED;
	routine->next 			= NULL;
	routine->parent 		= NULL;
}

static int codeMap_filter_section_executed(struct cm_section* section){
	struct cm_routine* cursor = section->routines;

	while(cursor != NULL){
		if (codeMap_filter_routine_executed(cursor)){
			return 1;
		}
		cursor = cursor->next;
	}

	return 0;
}

static int codeMap_filter_image_executed(struct cm_image* image){
	struct cm_section* cursor = image->sections;

	while(cursor != NULL){
		if (codeMap_filter_section_executed(cursor)){
			return 1;
		}
		cursor = cursor->next;
	}

	return 0;
}

static int codeMap_filter_section_whitelisted(struct cm_section* section){
	struct cm_routine* routine;

	for (routine = section->routines; routine != NULL; routine = routine->next){
		if (codeMap_filter_routine_whitelisted(routine)){
			break;
		}
	}
	if (routine == NULL){
		return 0;
	}

	return 1;
}

static int codeMap_filter_image_whitelisted(struct cm_image* image){
	struct cm_section* section;

	if (image->white_listed != CODEMAP_WHITELISTED){
		for (section = image->sections; section != NULL; section = section->next){
			if (codeMap_filter_section_whitelisted(section)){
				break;
			}
		}
		if (section == NULL){
			return 0;
		}
	}

	return 1;
}

int codeMap_check_address(struct codeMap* cm){
	struct cm_image* 	cursor_image;
	struct cm_section* 	cursor_section;
	struct cm_routine*	cursor_routine;

	printf("Checking code map address consistency: ");

	if (cm != NULL){
		cursor_image = cm->images;
		while(cursor_image != NULL){

			if (cursor_image->address_start > cursor_image->address_stop){
				printf("ERROR: image start address is greater than its stop address\n");
				return -1;
			}

			cursor_section = cursor_image->sections;
			while(cursor_section != NULL){

				if (cursor_section->address_start > cursor_section->address_stop){
					printf("ERROR: section start address is greater thant its stop address\n");
					return -1;
				}

				if (cursor_section->address_start < cursor_image->address_start || cursor_section->address_stop > cursor_image->address_stop){
					printf("ERROR: section can't exist outside of an image\n");
					return -1;
				}

				cursor_routine = cursor_section->routines;
				while(cursor_routine != NULL){

					if (cursor_routine->address_start > cursor_routine->address_stop){
						printf("ERROR: routine start address is greater than its stop address\n");
						return -1;
					}

					if (cursor_routine->address_start < cursor_section->address_start || cursor_routine->address_stop > cursor_section->address_stop){
						printf("ERROR: section can't exist outside of an image\n");
						return -1;
					}

					cursor_routine = cursor_routine->next;
				}

				cursor_section = cursor_section->next;
			}

			cursor_image = cursor_image->next;
		}
	}

	printf("SUCCESS\n");

	return 0;
}

struct cm_image* codeMap_search_image(struct codeMap* cm, ADDRESS address){
	struct cm_image* image_cursor;

	if (cm != NULL){
		image_cursor = cm->images;

		while(image_cursor != NULL){
			if (CODEMAP_IS_ADDRESS_IN_IMAGE(image_cursor, address)){
				return image_cursor;
			}

			image_cursor = image_cursor->next;
		}
	}

	return NULL;
}

struct cm_section* codeMap_search_section(struct codeMap* cm, ADDRESS address){
	struct cm_image* 	image;
	struct cm_section* 	section_cursor;

	if (cm != NULL){
		image = codeMap_search_image(cm, address);

		if(image != NULL){
			section_cursor = image->sections;

			while(section_cursor != NULL){
				if (CODEMAP_IS_ADDRESS_IN_SECTION(section_cursor, address)){
					return section_cursor;
				}

				section_cursor = section_cursor->next;
			}
		}
	}

	return NULL;
}

struct cm_routine* codeMap_search_routine(struct codeMap* cm, ADDRESS address){
	struct cm_section* 	section;
	struct cm_routine* 	routine_cursor;

	if (cm != NULL){
		section = codeMap_search_section(cm, address);

		if (section != NULL){
			routine_cursor = section->routines;

			while(routine_cursor != NULL){
				if (CODEMAP_IS_ADDRESS_IN_ROUTINE(routine_cursor, address)){
					return routine_cursor;
				}

				routine_cursor = routine_cursor->next;
			}
		}
	}

	return NULL;
}

void codeMap_fprint_address_info(struct codeMap* cm, ADDRESS address, FILE* file){
	struct cm_routine* rtn;

	if (cm != NULL && (rtn = codeMap_search_routine(cm, address)) != NULL){
		fprintf(file, "IMG:%s+" PRINTF_ADDR_SHORT " ", CODEMAP_ROUTINE_GET_IMAGE(rtn)->name, address - CODEMAP_ROUTINE_GET_IMAGE(rtn)->address_start);
		fprintf(file, "SEC:%s+" PRINTF_ADDR_SHORT " ", CODEMAP_ROUTINE_GET_SECTION(rtn)->name, address - CODEMAP_ROUTINE_GET_SECTION(rtn)->address_start);
		fprintf(file, "RTN:%s+" PRINTF_ADDR_SHORT, rtn->name, address - rtn->address_start);
	}
}

struct cm_routine* codeMap_search_symbol(struct codeMap* cm, struct cm_routine* last, const char* symbol){
	struct cm_image* 	image_cursor;
	struct cm_section* 	section_cursor;
	struct cm_routine* 	routine_cursor;

	if (last == NULL){
		for (image_cursor = cm->images; image_cursor; image_cursor = image_cursor->next){
			for (section_cursor = image_cursor->sections; section_cursor; section_cursor = section_cursor->next){
				for (routine_cursor = section_cursor->routines; routine_cursor; routine_cursor = routine_cursor->next){
					if (!strncmp(symbol, routine_cursor->name, CODEMAP_DEFAULT_NAME_SIZE)){
						return routine_cursor;
					}
				}
			}
		}
	}
	else{
		routine_cursor 	= last->next;
		section_cursor 	= last->parent;
		image_cursor 	= section_cursor->parent;

		for ( ; image_cursor; image_cursor = image_cursor->next, section_cursor = (image_cursor != NULL) ? image_cursor->sections : NULL){
			for ( ; section_cursor; section_cursor = section_cursor->next, routine_cursor = (section_cursor != NULL) ? section_cursor->routines : NULL){
				for ( ; routine_cursor; routine_cursor = routine_cursor->next){
					if (!strncmp(symbol, routine_cursor->name, CODEMAP_DEFAULT_NAME_SIZE)){
						return routine_cursor;
					}
				}
			}
		}
	}

	return NULL;
}

struct cm_routine* codeMap_search_approx_symbol(struct codeMap* cm, struct cm_routine* last, const char* symbol){
	struct cm_image* 	image_cursor;
	struct cm_section* 	section_cursor;
	struct cm_routine* 	routine_cursor;

	if (last == NULL){
		for (image_cursor = cm->images; image_cursor != NULL; image_cursor = image_cursor->next){
			for (section_cursor = image_cursor->sections; section_cursor != NULL; section_cursor = section_cursor->next){
				for (routine_cursor = section_cursor->routines; routine_cursor != NULL; routine_cursor = routine_cursor->next){
					if (strcasestr(routine_cursor->name, symbol) != NULL){
						return routine_cursor;
					}
				}
			}
		}
	}
	else{
		routine_cursor 	= last->next;
		section_cursor 	= last->parent;
		image_cursor 	= section_cursor->parent;

		for ( ; image_cursor != NULL; image_cursor = image_cursor->next, section_cursor = (image_cursor != NULL) ? image_cursor->sections : NULL, routine_cursor = (section_cursor != NULL) ? section_cursor->routines : NULL){
			for ( ; section_cursor != NULL; section_cursor = section_cursor->next, routine_cursor = (section_cursor != NULL) ? section_cursor->routines : NULL){
				for ( ; routine_cursor != NULL; routine_cursor = routine_cursor->next){
					if (strcasestr(routine_cursor->name, symbol) != NULL){
						return routine_cursor;
					}
				}
			}
		}
	}

	return NULL;
}

void codeMap_search_and_print_symbol(struct codeMap* cm, const char* symbol){
	struct cm_routine* 			ptr;
	struct multiColumnPrinter* 	printer;

	codeMap_create_printer(printer)

	for (ptr = codeMap_search_approx_symbol(cm, NULL, symbol); ptr != NULL; ptr = codeMap_search_approx_symbol(cm, ptr, symbol)){
		multiColumnPrinter_print(printer, ptr->parent->parent->name, ptr->parent->name, ptr->name, ptr->address_start, ptr->address_stop, codeMap_filter_routine_whitelisted(ptr), codeMap_filter_routine_executed(ptr), NULL);
	}

	multiColumnPrinter_delete(printer);
}

#endif

struct codeMap* codeMap_create(void){
	struct codeMap* cm = (struct codeMap*)malloc(sizeof(struct codeMap));
	if (cm == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
	}
	else{
		cm->images= NULL;

		cm->current_image = NULL;
		cm->current_section = NULL;
	}

	return cm;
}

int codeMap_add_image(struct codeMap* cm, ADDRESS address_start, ADDRESS address_stop, const char* name, char white_listed){
	struct cm_image* 	image;
	struct cm_image** 	cursor = &(cm->images);

	if (cm != NULL){
		image = (struct cm_image*)malloc(sizeof(struct cm_image));
		if (image == NULL){
			printf("ERROR: in %s, unable to allocate memory\n", __func__);
			return -1;
		}
		else{
			image->address_start = address_start;
			image->address_stop = address_stop;
			strncpy(image->name, name, CODEMAP_DEFAULT_NAME_SIZE);
			image->white_listed = white_listed;
			image->sections = NULL;
			image->next = NULL;
			image->parent = cm;

			while(*cursor != NULL){
				cursor = &((*cursor)->next);
			}
			*cursor = image;

			cm->current_image = image;
			cm->current_section = NULL;
		}
	}

	return 0;
}

int codeMap_add_section(struct codeMap* cm, ADDRESS address_start, ADDRESS address_stop, const char* name){
	struct cm_section* 	section;
	struct cm_section** cursor;

	if (cm != NULL){
		if (cm->current_image != NULL){
			section = (struct cm_section*)malloc(sizeof(struct cm_section));
			if (section == NULL){
				printf("ERROR: in %s, unable to allocate memory\n", __func__);
				return -1;
			}
			else{
				section->address_start = address_start;
				section->address_stop = address_stop;
				strncpy(section->name, name, CODEMAP_DEFAULT_NAME_SIZE);
				section->routines = NULL;
				section->next = NULL;
				section->parent = cm->current_image;

				cursor = &(cm->current_image->sections);
				while(*cursor != NULL){
					cursor = &((*cursor)->next);
				}
				*cursor = section;

				cm->current_section = section;
			}
		}
		else{
			printf("ERROR: in %s, current image is NULL\n", __func__);
			return -1;
		}
	}

	return 0;
}

struct cm_routine* codeMap_add_routine(struct codeMap* cm, ADDRESS address_start, ADDRESS address_stop, const char* name, char white_listed){
	struct cm_routine*	routine = NULL;
	struct cm_routine**	cursor;

	if (cm != NULL){
		if (cm->current_section != NULL){
			routine = (struct cm_routine*)malloc(sizeof(struct cm_routine));
			if (routine == NULL){
				printf("ERROR: in %s, unable to allocate memory\n", __func__);
			}
			else{
				routine->address_start = address_start;
				routine->address_stop = address_stop;
				strncpy(routine->name, name, CODEMAP_DEFAULT_NAME_SIZE);
				routine->white_listed = white_listed;
				routine->nb_execution = 0;
				routine->next = NULL;
				routine->parent = cm->current_section;

				cursor = &(cm->current_section->routines);
				while(*cursor != NULL){
					cursor = &((*cursor)->next);
				}
				*cursor = routine;
			}
		}
		else{
			printf("ERROR: in %s, current section is NULL\n", __func__);
		}
	}

	return routine;
}

void codeMap_delete(struct codeMap* cm){
	struct cm_routine* routine;
	struct cm_routine* tmp_routine;
	struct cm_section* section;
	struct cm_section* tmp_section;
	struct cm_image* image;
	struct cm_image* tmp_image;

	if (cm != NULL){
		image = cm->images;
		while(image != NULL){
			section = image->sections;
			while(section != NULL){
				routine = section->routines;
				while(routine != NULL){
					tmp_routine = routine->next;
					free(routine);
					routine = tmp_routine;
				}

				tmp_section = section->next;
				free(section);
				section = tmp_section;
			}

			tmp_image= image->next;
			free(image);
			image= tmp_image;
		}

		free(cm);
	}
}
