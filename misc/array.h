#ifndef ARRAY_H
#define ARRAY_H

#include <stdint.h>

#define ARRAY_DEFAULT_ALLOC_SIZE 	1024 	/* how to set this value properly ?? for best efficiency try some test */
#define ARRAY_DEFAULT_PAGE_SIZE 	131072 	/* same for this value - MUST be a multiple of ARRAY_DEFAULT_ALLOC_SIZE */

struct _array{
	char* 		buffer;
	uint32_t 	nb_allocated_byte;
	uint32_t 	nb_filled_byte;
	uint32_t	element_size;
};

struct _array* _array_create(uint32_t element_size);
void _array_init(struct _array* _array, uint32_t element_size);
int32_t _array_add(struct _array* _array, void* element);
int32_t _array_clone(struct _array* _array_src, struct _array* _array_dst);
void _array_empty(struct _array* _array);
void _array_clean(struct _array* _array);
void _array_delete(struct _array* _array);

#define _array_get(_array, index) ((_array).buffer + (index) * (_array).element_size)
#define _array_get_length(_array) ((_array).nb_filled_byte / (_array).element_size)

struct arrayPage{
	char* 			buffer;
	uint32_t 		nb_allocated_byte;
	uint32_t 		nb_filled_byte;
};

struct array{
	struct _array 	pages;
	char* 			buffer;
	uint32_t		nb_allocated_byte;
	uint32_t 		nb_filled_byte;
	uint32_t 		nb_element;
	uint32_t 		element_size;
};

struct array* array_create(uint32_t element_size);
int32_t array_init(struct array* array, uint32_t element_size);
int32_t array_add(struct array* array, void* element);
void* array_get(struct array* array, uint32_t index);
int32_t array_search_seq_up(struct array* array, uint32_t min_index, uint32_t max_index, void* key, int32_t(*compare)(void* element, void* key));
int32_t array_search_seq_down(struct array* array, uint32_t min_index, uint32_t max_index, void* key, int32_t(*compare)(void* element, void* key));
uint32_t* array_create_mapping(struct array* array, int32_t(*compare)(void* element1, void* element2));
int32_t array_clone(struct array* array_src, struct array* array_dst);
int32_t array_copy(struct array* array_src, struct array* array_dst, uint32_t offset, uint32_t nb_element);
void array_empty(struct array* array);
void array_clean(struct array* array);
void array_delete(struct array* array);

#define array_get_length(array) ((array)->nb_element)

#endif
