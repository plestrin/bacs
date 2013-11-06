#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "codeMap.h"

static void codeMap_print_routine_JSON(struct cm_routine* routine, FILE* file);
static void codeMap_print_section_JSON(struct cm_section* section, FILE* file);
static void codeMap_print_image_JSON(struct cm_image* image, FILE* file);

static void codeMap_print_routine(struct cm_routine* routine);
static void codeMap_print_section(struct cm_section* section);
static void codeMap_print_image(struct cm_image* image);


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

int codeMap_add_image(struct codeMap* cm, unsigned long address_start, unsigned long address_stop, const char* name){
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

int codeMap_add_section(struct codeMap* cm, unsigned long address_start, unsigned long address_stop, const char* name){
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

struct cm_routine* codeMap_add_routine(struct codeMap* cm, unsigned long address_start, unsigned long address_stop, const char* name){
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
				new_routine->nb_execution = 0;
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

void codeMap_print(struct codeMap* cm){
	struct cm_image* image;

	if (cm != NULL){
		printf("*** Code Map ***\n");

		image = cm->images;
		while(image != NULL){
			codeMap_print_image(image);
			image = image->next;
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
		fprintf(file, "{\"start\":\"%lx\",\"stop\":\"%lx\",\"name\":\"%s\"}", routine->address_start, routine->address_stop, routine->name);
	}
}

static void codeMap_print_section_JSON(struct cm_section* section, FILE* file){
	struct cm_routine* routine;

	if (section != NULL){
		/*fprintf(file, "{\"start\":\"%lx\",\"stop\":\"%lx\",\"name\":\"%s\",\"routine\":[", section->address_start, section->address_stop, section->name);*/
		fprintf(file, "{\"name\":\"%s\",\"routine\":[", section->name);

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
		/*fprintf(file, "{\"start\":\"%lx\",\"stop\":\"%lx\",\"name\":\"%s\",\"section\":[", image->address_start, image->address_stop, image->name);*/
		fprintf(file, "{\"name\":\"%s\",\"section\":[", image->name);

		section = image->sections;
		while(section != NULL){
			codeMap_print_section_JSON(section, file);
			section = section->next;
			if (section != NULL){
				fprintf(file, ",");
			}
			else{
				fprintf(file, "]}");
			}
		}		
	}
}

static void codeMap_print_routine(struct cm_routine* routine){
	if (routine != NULL){
		printf("\t\t\tName: %s, start: 0x%lx, stop: 0x%lx\n", routine->name, routine->address_start, routine->address_stop);
	}
}

static void codeMap_print_section(struct cm_section* section){
	struct cm_routine* routine;

	if (section != NULL){
		/*printf("\t\tName: %s, start: 0x%lx, stop: 0x%lx\n", section->name, section->address_start, section->address_stop);*/
		printf("\t\tName: %s\n", section->name);

		routine = section->routines;
		while(routine != NULL){
			codeMap_print_routine(routine);
			routine = routine->next;
		}		
	}
}

static void codeMap_print_image(struct cm_image* image){
	struct cm_section* section;

	if (image != NULL){
		/*printf("\tName: %s, start: 0x%lx, stop:0x%lx\n", image->name, image->address_start, image->address_stop);*/
		printf("\tName: %s\n", image->name);

		section = image->sections;
		while(section != NULL){
			codeMap_print_section(section);
			section = section->next;
		}		
	}
}

void codeMap_clean_image(struct cm_image* image){
	image->address_start 	= 0;
	image->address_stop 	= 0;
	memset(image->name, '\0', CODEMAP_DEFAULT_NAME_SIZE);
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
	routine->next 			= NULL;
	routine->parent 		= NULL;
}