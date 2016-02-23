#ifndef ARRAY_H
#define ARRAY_H

#include <stdint.h>

#define _ARRAY_DEFAULT_ALLOC_PAGE 	64 		/* how to set this value properly ?? for best efficiency try some test */
#define ARRAY_DEFAULT_ALLOC_SIZE 	4096 	/* how to set this value properly ?? for best efficiency try some test */
#define ARRAY_DEFAULT_PAGE_SIZE 	131072 	/* same for this value - MUST be a multiple of ARRAY_DEFAULT_ALLOC_SIZE - It is faster to choose a power of two */

struct arrayPage{
	char* 				buffer;
	uint32_t 			nb_allocated_byte;
	uint32_t 			nb_filled_byte;
};

struct _array{
	struct arrayPage* 	buffer;
	uint32_t 			nb_allocated_page;
	uint32_t 			nb_filled_page;
};

struct _array* _array_create(void);

#define _array_init(_array) 					\
	(_array)->buffer 				= NULL; 	\
	(_array)->nb_allocated_page 	= 0; 		\
	(_array)->nb_filled_page 		= 0;

int32_t _array_add(struct _array* _array, void* element);

#define _array_get(_array, index) ((_array).buffer + index)
#define _array_get_length(_array) ((_array).nb_filled_page)

int32_t _array_clone(struct _array* _array_src, struct _array* _array_dst);

#define _array_empty(_array) 					\
	(_array)->nb_allocated_page 	= 0; 		\
	(_array)->nb_filled_page 		= 0; 		\
												\
	if ((_array)->buffer != NULL){ 				\
		free((_array)->buffer); 				\
		(_array)->buffer = NULL; 				\
	}

#define _array_clean(_array) 					\
	if ((_array)->buffer != NULL){ 				\
		free((_array)->buffer); 				\
	}

#define _array_delete(_array) 					\
	_array_clean(_array);						\
	free(_array);

struct array{
	struct _array 		pages;
	char* 				buffer;
	uint32_t			nb_allocated_byte;
	uint32_t 			nb_filled_byte;
	uint32_t 			nb_element;
	uint32_t 			element_size;
	uint32_t 			nb_element_per_page;
};

struct array* array_create(uint32_t element_size);
int32_t array_init(struct array* array, uint32_t element_size);
int32_t array_add(struct array* array, const void* element);

static inline void* array_get(const struct array* array, uint32_t index){
	struct arrayPage* 	page;
	uint32_t 			local_offset;

	local_offset = (index % array->nb_element_per_page) * array->element_size;

	if (_array_get_length(array->pages) * array->nb_element_per_page <= index){
		return array->buffer + local_offset;
	}
	else{
		page = (struct arrayPage*)_array_get(array->pages, index / array->nb_element_per_page);
		return page->buffer + local_offset;
	}
}

#define array_get_length(array) ((array)->nb_element)

uint32_t* array_create_mapping(struct array* array, int32_t(*compare)(void* element1, void* element2));
int32_t array_clone(struct array* array_src, struct array* array_dst);
int32_t array_copy(struct array* array_src, struct array* array_dst, uint32_t offset, uint32_t nb_element);
void array_empty(struct array* array);
void array_clean(struct array* array);

#define array_delete(array) 					\
	if (array != NULL){ 						\
		array_clean(array); 					\
		free(array); 							\
	}

#endif
