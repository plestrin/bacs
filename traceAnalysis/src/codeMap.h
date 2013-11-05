#ifndef CODEMAP_H
#define CODEMAP_H

#define CODEMAP_DEFAULT_NAME_SIZE 256

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
	unsigned int 		nb_execution;
	struct cm_routine*	next;
	struct cm_section*	parent;
};

struct codeMap* codeMap_create();
int codeMap_add_image(struct codeMap* cm, unsigned long address_start, unsigned long address_stop, const char* name);
int codeMap_add_section(struct codeMap* cm, unsigned long address_start, unsigned long address_stop, const char* name);
struct cm_routine* codeMap_add_routine(struct codeMap* cm, unsigned long address_start, unsigned long address_stop, const char* name);
void codeMap_print_JSON(struct codeMap* cm, FILE* file);
void codeMap_delete(struct codeMap* cm);

#endif