#ifndef ARRAY_H
#define ARRAY_H

#include <stdint.h>

#define ARRAY_DEFAULT_ALLOC_SIZE 	1024
#define ARRAY_DEFAULT_PAGE_SIZE 	131072

struct _array{
	char* 		buffer;
	uint32_t 	nb_allocated_byte;
	uint32_t 	nb_filled_byte;
	uint32_t	element_size;
};

struct _array* _array_create(uint32_t element_size);
void _array_init(struct _array* _array, uint32_t element_size);
int _array_add(struct _array* _array, void* element);
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
int array_init(struct array* array, uint32_t element_size);
int array_add(struct array* array, void* element);
void* array_get(struct array* array, uint32_t index);
void array_clean(struct array* array);
void array_delete(struct array* array);

#define array_get_length(array) ((array)->nb_element)

#endif
