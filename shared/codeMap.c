#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "codeMap.h"
#include "multiColumn.h"

#ifdef WIN32
#include "windowsComp.h"
#endif

static void codeMap_print_routine_JSON(struct cm_routine* routine, FILE* file);
static void codeMap_print_section_JSON(struct cm_section* section, FILE* file);
static void codeMap_print_image_JSON(struct cm_image* image, FILE* file);

static void codeMap_print_routine(struct multiColumnPrinter* printer, struct cm_routine* routine, int filter);
static void codeMap_print_section(struct multiColumnPrinter* printer, struct cm_section* section, int filter);
static void codeMap_print_image(struct multiColumnPrinter* printer, struct cm_image* image, int filter);

static int codeMap_filter_routine_executed(struct cm_routine* routine);
static int codeMap_filter_section_executed(struct cm_section* section);
static int codeMap_filter_image_executed(struct cm_image* image);

static int codeMap_filter_routine_whitelisted(struct cm_routine* routine);
static int codeMap_filter_section_whitelisted(struct cm_section* section);
static int codeMap_filter_image_whitelisted(struct cm_image* image);


struct codeMap* codeMap_create(){
	struct codeMap* cm = (struct codeMap*)malloc(sizeof(struct codeMap));
	if (cm == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
	}
	else{
		cm->images= NULL;

		cm->current_image = NULL;
		cm->current_section = NULL;

		cm->memory_consumption = sizeof(struct codeMap);
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

			cm->memory_consumption += sizeof(struct cm_image);
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

				cm->memory_consumption += sizeof(struct cm_section);
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

				cm->memory_consumption += sizeof(struct cm_routine);
			}
		}
		else{
			printf("ERROR: in %s, current section is NULL\n", __func__);
		}
	}

	return routine;
}

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

			cm->memory_consumption += sizeof(struct cm_image);
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

				cm->memory_consumption += sizeof(struct cm_section);
			}
		}
		else{
			printf("ERROR: in %s, current image is NULL\n", __func__);
			return -1;
		}
	}

	return 0;
}

int codeMAp_add_static_routine(struct codeMap* cm, struct cm_routine* routine){
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

				cm->memory_consumption += sizeof(struct cm_routine);
			}
		}
		else{
			printf("ERROR: in %s, current section is NULL\n", __func__);
			return -1;
		}
	}

	return 0;
}

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

void codeMap_print(struct codeMap* cm, char* str_filter){
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
		else{
			printf("*** Code Map ***\n");
		}

		printer = multiColumnPrinter_create(stdout, 6, NULL, NULL, NULL);
		if (printer != NULL){
			multiColumnPrinter_set_column_size(printer, 0, 38);
			multiColumnPrinter_set_column_size(printer, 1, 7);
			multiColumnPrinter_set_column_size(printer, 2, 24);
			multiColumnPrinter_set_column_size(printer, 3, 10);
			multiColumnPrinter_set_column_size(printer, 4, 10);
			multiColumnPrinter_set_column_size(printer, 5, 6);

			#if defined ARCH_32
			multiColumnPrinter_set_column_type(printer, 3, MULTICOLUMN_TYPE_HEX_32);
			multiColumnPrinter_set_column_type(printer, 4, MULTICOLUMN_TYPE_HEX_32);
			#elif defined ARCH_64
			multiColumnPrinter_set_column_type(printer, 3, MULTICOLUMN_TYPE_HEX_64);
			multiColumnPrinter_set_column_type(printer, 4, MULTICOLUMN_TYPE_HEX_64);
			#else
			#error Please specify an architecture {ARCH_32 or ARCH_64}
			#endif

			multiColumnPrinter_set_title(printer, 0, (char*)"IMAGE");
			multiColumnPrinter_set_title(printer, 1, (char*)"SECTION");
			multiColumnPrinter_set_title(printer, 2, (char*)"ROUTINE");
			multiColumnPrinter_set_title(printer, 3, (char*)"START @");
			multiColumnPrinter_set_title(printer, 4, (char*)"STOP @");
			multiColumnPrinter_set_title(printer, 5, (char*)"WHITEL");
			multiColumnPrinter_print_header(printer);

			image = cm->images;
			while(image != NULL){
				codeMap_print_image(printer, image, filter);
				image = image->next;
			}

			multiColumnPrinter_delete(printer);
		}
		else{
			printf("ERROR: in %s, unable to create multi column printer\n", __func__);
		}
	}
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

static void codeMap_print_routine_JSON(struct cm_routine* routine, FILE* file){
	if (routine != NULL){
		if (routine->white_listed == CODEMAP_WHITELISTED){
			#if defined ARCH_32
			fprintf(file, "{\"start\":\"%x\",\"stop\":\"%x\",\"name\":\"%s\",\"whl\":true,\"exe\":%u}", routine->address_start, routine->address_stop, routine->name, routine->nb_execution);
			#elif defined ARCH_64
			fprintf(file, "{\"start\":\"%llx\",\"stop\":\"%llx\",\"name\":\"%s\",\"whl\":true,\"exe\":%u}", routine->address_start, routine->address_stop, routine->name, routine->nb_execution);
			#else
			#error Please specify an architecture {ARCH_32 or ARCH_64}
			#endif
		}
		else{
			#if defined ARCH_32
			fprintf(file, "{\"start\":\"%x\",\"stop\":\"%x\",\"name\":\"%s\",\"whl\":false,\"exe\":%u}", routine->address_start, routine->address_stop, routine->name, routine->nb_execution);
			#elif defined ARCH_64
			fprintf(file, "{\"start\":\"%llx\",\"stop\":\"%llx\",\"name\":\"%s\",\"whl\":false,\"exe\":%u}", routine->address_start, routine->address_stop, routine->name, routine->nb_execution);
			#else
			#error Please specify an architecture {ARCH_32 or ARCH_64}
			#endif
		}
	}
}

static void codeMap_print_section_JSON(struct cm_section* section, FILE* file){
	struct cm_routine* routine;

	if (section != NULL){
		#if defined ARCH_32
		fprintf(file, "{\"start\":\"%x\",\"stop\":\"%x\",\"name\":\"%s\",\"routine\":[", section->address_start, section->address_stop, section->name);
		#elif defined ARCH_64
		fprintf(file, "{\"start\":\"%llx\",\"stop\":\"%llx\",\"name\":\"%s\",\"routine\":[", section->address_start, section->address_stop, section->name);
		#else
		#error Please specify an architecture {ARCH_32 or ARCH_64}
		#endif

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
			#if defined ARCH_32
			#ifdef WIN32
			fprintf(file, "{\"start\":\"%x\",\"stop\":\"%x\",\"name\":\"%s\",\"whl\":true,\"section\":[", image->address_start, image->address_stop, windowsComp_sanitize_path(image->name));
			#else
			fprintf(file, "{\"start\":\"%x\",\"stop\":\"%x\",\"name\":\"%s\",\"whl\":true,\"section\":[", image->address_start, image->address_stop, image->name);
			#endif
			#elif defined ARCH_64
			#ifdef WIN32
			fprintf(file, "{\"start\":\"%llx\",\"stop\":\"%llx\",\"name\":\"%s\",\"whl\":true,\"section\":[", image->address_start, image->address_stop, windowsComp_sanitize_path(image->name));
			#else
			fprintf(file, "{\"start\":\"%llx\",\"stop\":\"%llx\",\"name\":\"%s\",\"whl\":true,\"section\":[", image->address_start, image->address_stop, image->name);
			#endif
			#else
			#error Please specify an architecture {ARCH_32 or ARCH_64}
			#endif
		}
		else{
			#if defined ARCH_32
			#ifdef WIN32
			fprintf(file, "{\"start\":\"%x\",\"stop\":\"%x\",\"name\":\"%s\",\"whl\":false,\"section\":[", image->address_start, image->address_stop, windowsComp_sanitize_path(image->name));
			#else
			fprintf(file, "{\"start\":\"%x\",\"stop\":\"%x\",\"name\":\"%s\",\"whl\":false,\"section\":[", image->address_start, image->address_stop, image->name);
			#endif
			#elif defined ARCH_64
			#ifdef WIN32
			fprintf(file, "{\"start\":\"%llx\",\"stop\":\"%llx\",\"name\":\"%s\",\"whl\":false,\"section\":[", image->address_start, image->address_stop, windowsComp_sanitize_path(image->name));
			#else
			fprintf(file, "{\"start\":\"%llx\",\"stop\":\"%llx\",\"name\":\"%s\",\"whl\":false,\"section\":[", image->address_start, image->address_stop, image->name);
			#endif
			#else
			#error Please specify an architecture {ARCH_32 or ARCH_64}
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

static void codeMap_print_routine(struct multiColumnPrinter* printer, struct cm_routine* routine, int filter){
	if (routine != NULL){
		if (filter & CODEMAP_FILTER_EXECUTED){
			if (!codeMap_filter_routine_executed(routine)){
				return;
			}
		}
		if (filter & CODEMAP_FILTER_WHITELIST){
			if (codeMap_filter_routine_whitelisted(routine)){
				return;
			}
		}

		if (routine->white_listed == CODEMAP_WHITELISTED){
			multiColumnPrinter_print(printer, " ", " ", routine->name, routine->address_start, routine->address_stop, "yes", NULL);
		}
		else{
			multiColumnPrinter_print(printer, " ", " ", routine->name, routine->address_start, routine->address_stop, "no", NULL);
		}
	}
}

static void codeMap_print_section(struct multiColumnPrinter* printer, struct cm_section* section, int filter){
	struct cm_routine* 	routine;

	if (section != NULL){
		if (filter & CODEMAP_FILTER_EXECUTED){
			if (!codeMap_filter_section_executed(section)){
				return;
			}
		}
		if (filter & CODEMAP_FILTER_WHITELIST){
			if (codeMap_filter_section_whitelisted(section)){
				return;
			}
		}

		multiColumnPrinter_print(printer, " ", section->name, " ", section->address_start, section->address_stop, " ", NULL);

		routine = section->routines;
		while(routine != NULL){
			codeMap_print_routine(printer, routine, filter);
			routine = routine->next;
		}		
	}
}

static void codeMap_print_image(struct multiColumnPrinter* printer, struct cm_image* image, int filter){
	struct cm_section* 	section;

	if (image != NULL){
		if (filter & CODEMAP_FILTER_EXECUTED){
			if (!codeMap_filter_image_executed(image)){
				return;
			}
		}
		if (filter & CODEMAP_FILTER_WHITELIST){
			if (codeMap_filter_image_whitelisted(image)){
				return;
			}
		}

		if (image->white_listed == CODEMAP_WHITELISTED){
			multiColumnPrinter_print(printer, image->name, " ", " ", image->address_start, image->address_stop, "yes", NULL);
		}
		else{
			multiColumnPrinter_print(printer, image->name, " ", " ", image->address_start, image->address_stop, "no", NULL);
		}

		section = image->sections;
		while(section != NULL){
			codeMap_print_section(printer, section, filter);
			section = section->next;
		}

		multiColumnPrinter_print_horizontal_separator(printer);
	}
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

static int codeMap_filter_routine_executed(struct cm_routine* routine){
	return routine->nb_execution;
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

static int codeMap_filter_routine_whitelisted(struct cm_routine* routine){
	return (routine->white_listed == CODEMAP_WHITELISTED);
}

static int codeMap_filter_section_whitelisted(struct cm_section* section){
	struct cm_routine* cursor = section->routines;

	while(cursor != NULL){
		if (!codeMap_filter_routine_whitelisted(cursor)){
			return 0;
		}
		cursor = cursor->next;
	}

	return 1;
}

static int codeMap_filter_image_whitelisted(struct cm_image* image){
	if (image->white_listed == CODEMAP_WHITELISTED){
		return 1;
	}
	else{
		struct cm_section* cursor = image->sections;

		while(cursor != NULL){
			if (!codeMap_filter_section_whitelisted(cursor)){
				return 0;
			}
			cursor = cursor->next;
		}

		return 1;
	}
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

int codeMap_is_instruction_whiteListed(struct codeMap* cm, ADDRESS address){
	struct cm_image* 	image_cursor;
	struct cm_section* 	section_cursor;
	struct cm_routine* 	routine_cursor;

	if (cm != NULL){
		image_cursor = cm->images;

		while(image_cursor != NULL){
			if (CODEMAP_IS_ADDRESS_IN_IMAGE(image_cursor, address)){
				if (image_cursor->white_listed == CODEMAP_WHITELISTED){
					return CODEMAP_WHITELISTED;
				}

				section_cursor = image_cursor->sections;
				while(section_cursor != NULL){
					if (CODEMAP_IS_ADDRESS_IN_SECTION(section_cursor, address)){
						routine_cursor = section_cursor->routines;

						while(routine_cursor != NULL){
							if (CODEMAP_IS_ADDRESS_IN_ROUTINE(routine_cursor, address)){
								return routine_cursor->white_listed;
							}

							routine_cursor = routine_cursor->next;
						}
					}
					break;

					section_cursor = section_cursor->next;
				}
				break;
			}

			image_cursor = image_cursor->next;
		}
	}

	return CODEMAP_NOT_WHITELISTED;
}