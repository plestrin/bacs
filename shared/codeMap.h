#ifndef CODEMAP_H
#define CODEMAP_H

#include "address.h"

#define CODEMAP_DEFAULT_NAME_SIZE 		512

#define CODEMAP_NOT_WHITELISTED			0
#define CODEMAP_WHITELISTED				1

#define CODEMAP_FILTER_WHITELIST		0x00000001
#define CODEMAP_FILTER_EXECUTED			0x00000002

#define CODEMAP_FILTER_WHITELIST_CMD 	'W'
#define CODEMAP_FILTER_EXECUTED_CMD 	'E'

struct codeMap{
	struct cm_image*	images;

	struct cm_image*	current_image;
	struct cm_section*	current_section;
};

struct cm_image{
	ADDRESS 			address_start;
	ADDRESS 			address_stop;
	char				name[CODEMAP_DEFAULT_NAME_SIZE];
	char				white_listed;
	struct cm_section*	sections;
	struct cm_image*	next;
	struct codeMap*		parent;
};

struct cm_section{
	ADDRESS 			address_start;
	ADDRESS 			address_stop;
	char				name[CODEMAP_DEFAULT_NAME_SIZE];
	struct cm_routine*	routines;
	struct cm_section*	next;
	struct cm_image*	parent;
};

struct cm_routine{
	ADDRESS 			address_start;
	ADDRESS				address_stop;
	char				name[CODEMAP_DEFAULT_NAME_SIZE];
	char				white_listed;
	unsigned int 		nb_execution;
	struct cm_routine*	next;
	struct cm_section*	parent;
};

#ifdef __PIN__

int codeMap_is_instruction_whiteListed(struct codeMap* cm, ADDRESS address);

#ifdef __linux__
int codeMap_add_vdso(struct codeMap* cm, char white_listed);
#endif

#define CODEMAP_INCREMENT_ROUTINE_EXE(rtn) ((rtn)->nb_execution ++)

void codeMap_print_JSON(struct codeMap* cm, FILE* file);

#else

int codeMap_check_address(struct codeMap* cm);
void codeMap_print(struct codeMap* cm, const char* str_filter);

void codeMap_clean_image(struct cm_image* image);
void codeMap_clean_section(struct cm_section* section);
void codeMap_clean_routine(struct cm_routine* routine);

int codeMap_add_static_image(struct codeMap* cm, struct cm_image* image);
int codeMap_add_static_section(struct codeMap* cm, struct cm_section* section);
int codeMap_add_static_routine(struct codeMap* cm, struct cm_routine* routine);

struct cm_image* codeMap_search_image(struct codeMap* cm, ADDRESS address);
struct cm_section* codeMap_search_section(struct codeMap* cm, ADDRESS address);
struct cm_routine* codeMap_search_routine(struct codeMap* cm, ADDRESS address);

void codeMap_fprint_address_info(struct codeMap* cm, ADDRESS address, FILE* file);

struct cm_routine* codeMap_search_symbol(struct codeMap* cm, struct cm_routine* last, const char* symbol);
struct cm_routine* codeMap_search_approx_symbol(struct codeMap* cm, struct cm_routine* last, const char* symbol);

void codeMap_search_and_print_symbol(struct codeMap* cm, const char* symbol);

#endif

struct codeMap* codeMap_create(void);

#define CODEMAP_ROUTINE_GET_SECTION(rtn) 			((rtn)->parent)
#define CODEMAP_ROUTINE_GET_IMAGE(rtn) 				((rtn)->parent->parent)
#define CODEMAP_SECTION_GET_IMAGE(sec) 				((sec)->parent)

#define CODEMAP_IS_ADDRESS_IN_ROUTINE(rtn, addr)  	((rtn)->address_start <= (addr) && (rtn)->address_stop > (addr))
#define CODEMAP_IS_ADDRESS_IN_SECTION(sec, addr) 	((sec)->address_start <= (addr) && (sec)->address_stop >= (addr))
#define CODEMAP_IS_ADDRESS_IN_IMAGE(img, addr) 		((img)->address_start <= (addr) && (img)->address_stop >= (addr))

int codeMap_add_image(struct codeMap* cm, ADDRESS address_start, ADDRESS address_stop, const char* name, char white_listed);
int codeMap_add_section(struct codeMap* cm, ADDRESS address_start, ADDRESS address_stop, const char* name);
struct cm_routine* codeMap_add_routine(struct codeMap* cm, ADDRESS address_start, ADDRESS address_stop, const char* name, char white_listed);

void codeMap_delete(struct codeMap* cm);

#endif
