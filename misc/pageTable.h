#ifndef PAGETABLE_H
#define PAGETABLE_H

#include <stdint.h>

#define PAGETABLE_TRACK_MEMORY_CONSUMPTION

struct pageData{
	uint32_t 			nb_element;
	uint32_t 			set_vector[8];
	char 				data[256];
};

struct pageIndex{
	uint32_t 			nb_element;
	void* 				pages[256];
};

struct pageTable{
	size_t 				data_size;
	struct pageIndex* 	main_pages[256];
	#ifdef PAGETABLE_TRACK_MEMORY_CONSUMPTION
	size_t 				memory_consumption;
	#endif
};

struct pageTable* pageTable_create(size_t data_size);
int32_t pageTable_init(struct pageTable* pt, size_t data_size);

int32_t pageTable_add_element(struct pageTable* pt, uint32_t index, void* data);
void* pageTable_get_element(struct pageTable* pt, uint32_t index);
void* pageTable_get_or_add_element(struct pageTable* pt, uint32_t index, void* data);
void* pageTable_get_first(struct pageTable* pt, uint32_t* index);
void* pageTable_get_next(struct pageTable* pt, uint32_t* index);
void* pageTable_get_last(struct pageTable* pt, uint32_t* index);
void* pageTable_get_prev(struct pageTable* pt, uint32_t* index);
void pageTable_remove_element(struct pageTable* pt, uint32_t index);

void pageTable_clean(struct pageTable* pt);
void pageTable_delete(struct pageTable* pt);

#ifdef PAGETABLE_TRACK_MEMORY_CONSUMPTION
#define pageTable_get_memory_consumption(pt) ((pt)->memory_consumption)
#endif

#endif