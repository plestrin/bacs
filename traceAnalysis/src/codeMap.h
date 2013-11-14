#ifndef CODEMAP_H
#define CODEMAP_H

#define CODEMAP_DEFAULT_NAME_SIZE 	256

#define CODEMAP_NOT_WHITELISTED		0
#define CODEMAP_WHITELISTED			1


#define CODEMAP_FILTER_WHITELIST	0x00000001
#define CODEMAP_FILTER_EXECUTED		0x00000002

#define CODEMAP_TAB_LENGTH			8 /* warning this is quite messy - but it'll do the work for now  */


struct codeMap{
	struct cm_image*	images;

	struct cm_image*	current_image;
	struct cm_section*	current_section;

	long 				memory_consumption;
};

struct cm_image{
	unsigned long 		address_start;
	unsigned long 		address_stop;
	char				name[CODEMAP_DEFAULT_NAME_SIZE];
	char				white_listed;
	struct cm_section*	sections;
	struct cm_image*	next;
	struct codeMap*		parent;
};

struct cm_section{
	unsigned long 		address_start;
	unsigned long 		address_stop;
	char				name[CODEMAP_DEFAULT_NAME_SIZE];
	struct cm_routine*	routines;
	struct cm_section*	next;
	struct cm_image*	parent;
};

struct cm_routine{
	unsigned long 		address_start;
	unsigned long		address_stop;
	char				name[CODEMAP_DEFAULT_NAME_SIZE];
	char				white_listed;
	unsigned int 		nb_execution;
	struct cm_routine*	next;
	struct cm_section*	parent;
};

struct codeMap* codeMap_create();
int codeMap_add_image(struct codeMap* cm, unsigned long address_start, unsigned long address_stop, const char* name, char white_listed);
int codeMap_add_section(struct codeMap* cm, unsigned long address_start, unsigned long address_stop, const char* name);
struct cm_routine* codeMap_add_routine(struct codeMap* cm, unsigned long address_start, unsigned long address_stop, const char* name, char white_listed);
int codeMap_check_address(struct codeMap* cm);
void codeMap_print_JSON(struct codeMap* cm, FILE* file);
void codeMap_print(struct codeMap* cm, int filter);
void codeMap_delete(struct codeMap* cm);

void codeMap_clean_image(struct cm_image* image);
void codeMap_clean_section(struct cm_section* section);
void codeMap_clean_routine(struct cm_routine* routine);


int codeMap_add_static_image(struct codeMap* cm, struct cm_image* image);
int codeMap_add_static_section(struct codeMap* cm, struct cm_section* section);
int codeMAp_add_static_routine(struct codeMap* cm, struct cm_routine* routine);

struct cm_image* codeMap_search_image(struct codeMap* cm, unsigned long address);
struct cm_section* codeMap_search_section(struct codeMap* cm, unsigned long address);
struct cm_routine* codeMap_search_routine(struct codeMap* cm, unsigned long address);

int codeMap_is_instruction_whiteListed(struct codeMap* cm, unsigned long address);

#define CODEMAP_INCREMENT_ROUTINE_EXE(rtn) ((rtn)->nb_execution ++)


#endif