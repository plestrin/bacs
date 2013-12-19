#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "array.h"

struct _array* _array_create(uint32_t element_size){
	struct _array* _array;

	_array = (struct _array*)malloc(sizeof(struct _array));
	if (_array == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
	}
	else{
		_array_init(_array, element_size);
	}

	return _array;
}

void _array_init(struct _array* _array, uint32_t element_size){
	if (_array != NULL){
		_array->buffer 				= NULL;
		_array->nb_allocated_byte 	= 0;
		_array->nb_filled_byte 		= 0;
		_array->element_size 		= element_size;
	}
}

int32_t _array_add(struct _array* _array, void* element){
	int32_t 	result = -1;
	char* 		buffer;
	uint32_t	nb_allocated_byte;

	if (_array != NULL){
		nb_allocated_byte = _array->nb_allocated_byte;
		if (_array->nb_filled_byte + _array->element_size > nb_allocated_byte){
			while (_array->nb_filled_byte + _array->element_size > nb_allocated_byte){
				nb_allocated_byte += ARRAY_DEFAULT_ALLOC_SIZE;
			}

			if (_array->buffer == NULL){
				buffer = (char*)malloc(nb_allocated_byte);
			}
			else{
				buffer = (char*)realloc(_array->buffer, nb_allocated_byte);
			}
			if (buffer == NULL){
				printf("ERROR: in %s, unable to malloc/realloc memory\n", __func__);
				return result;
			}

			_array->buffer = buffer;
			_array->nb_allocated_byte = nb_allocated_byte;
		}

		memcpy(_array->buffer + _array->nb_filled_byte, element, _array->element_size);
		result = _array->nb_filled_byte / _array->element_size;
		_array->nb_filled_byte += _array->element_size;		
	}

	return result;
}

int32_t _array_clone(struct _array* _array_src, struct _array* _array_dst){
	if (_array_src->nb_allocated_byte > 0){
		_array_dst->buffer = (char*)malloc(_array_src->nb_allocated_byte);
		if (_array_dst->buffer == NULL){
			printf("ERROR: in %s, unable to allocate memory\n", __func__);
			return -1;
		}
		memcpy(_array_dst->buffer, _array_src->buffer, _array_src->nb_filled_byte);
	}
	else{
		_array_dst->buffer = NULL;
	}

	_array_dst->nb_allocated_byte 	= _array_src->nb_allocated_byte;
	_array_dst->nb_filled_byte		= _array_src->nb_filled_byte;
	_array_dst->element_size 		= _array_src->element_size;

	return 0;
}

void _array_empty(struct _array* _array){
	_array->nb_allocated_byte 	= 0;
	_array->nb_filled_byte 		= 0;
	
	if (_array->buffer != NULL){
		free(_array->buffer);
		_array->buffer = NULL;
	}
}

void _array_clean(struct _array* _array){
	if (_array != NULL){
		if (_array->buffer != NULL){
			free(_array->buffer);
		}
	}
}

void _array_delete(struct _array* _array){
	if (_array != NULL){
		_array_clean(_array);
		free(_array);
	}
}

struct array* array_create(uint32_t element_size){
	struct array* array;

	array = (struct array*)malloc(sizeof(struct array));
	if (array == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
	}
	else{
		if (array_init(array, element_size) != 0){
			free(array);
			array = NULL;
		}
	}

	return array;
}

int32_t array_init(struct array* array, uint32_t element_size){
	int32_t result = -1;

	if (array != NULL){
		_array_init(&(array->pages), sizeof(struct arrayPage));

		array->buffer = (char*)malloc(ARRAY_DEFAULT_ALLOC_SIZE);
		if (array->buffer != NULL){
			array->nb_allocated_byte 	= ARRAY_DEFAULT_ALLOC_SIZE;
			array->nb_filled_byte 		= 0;
			array->nb_element 			= 0;
			array->element_size 		= element_size;

			if (array->element_size > ARRAY_DEFAULT_PAGE_SIZE){
				printf("WARNING: in %s, element size is larger than a page\n", __func__);
			}

			result = 0;
		}
		else{
			printf("ERROR: in %s, unable to allocate memory\n", __func__);
			_array_clean(&(array->pages));
			result = -1;
		}
	}

	return result;
}

int32_t array_add(struct array* array, void* element){
	int32_t 			result = -1;
	char* 				buffer;
	uint32_t 			nb_allocated_byte;
	struct arrayPage 	page;

	if (array != NULL){
		nb_allocated_byte = array->nb_allocated_byte;
		if (array->nb_filled_byte + array->element_size > nb_allocated_byte){
			while (array->nb_filled_byte + array->element_size > nb_allocated_byte){
				nb_allocated_byte += array->element_size;
			}

			if (nb_allocated_byte > ARRAY_DEFAULT_PAGE_SIZE){
				page.buffer = array->buffer;
				page.nb_allocated_byte = array->nb_allocated_byte;
				page.nb_filled_byte = array->nb_filled_byte;

				if (_array_add(&(array->pages), &page) < 0){
					printf("ERROR: in %s, unable to archive page\n", __func__);
					return result;
				}

				nb_allocated_byte = ARRAY_DEFAULT_ALLOC_SIZE;
				array->nb_filled_byte = 0;

				while (array->nb_filled_byte + array->element_size > nb_allocated_byte){
					nb_allocated_byte += ARRAY_DEFAULT_ALLOC_SIZE;
				}

				array->buffer = (char*)malloc(nb_allocated_byte);
				if (array->buffer == NULL){
					printf("ERROR: in %s, unable to realloc memory\n", __func__);
					return result;
				}

				array->nb_allocated_byte = nb_allocated_byte;
			}
			else{
				buffer = (char*)realloc(array->buffer, nb_allocated_byte);
				if (buffer == NULL){
					printf("ERROR: in %s, unable to realloc memory\n", __func__);
					return result;
				}

				array->buffer = buffer;
				array->nb_allocated_byte = nb_allocated_byte;
			}
		}

		memcpy(array->buffer + array->nb_filled_byte, element, array->element_size);
		result = array->nb_element;
		array->nb_filled_byte += array->element_size;
		array->nb_element ++;
	}

	return result;
}

void* array_get(struct array* array, uint32_t index){
	struct arrayPage* 	page;
	uint32_t 			nb_element_per_page;

	nb_element_per_page = ARRAY_DEFAULT_PAGE_SIZE / array->element_size;
	if (_array_get_length(array->pages) * nb_element_per_page <= index){
		return array->buffer + (index % nb_element_per_page) * array->element_size;
	}
	else{
		page = (struct arrayPage*)_array_get(array->pages, index / nb_element_per_page);
		return page->buffer + (index % nb_element_per_page) * array->element_size;
	}
}

int32_t array_search_seq_up(struct array* array, uint32_t min_index, uint32_t max_index, void* key, int32_t(*compare)(void* element, void* key)){
	struct arrayPage* 	page;
	uint32_t 			nb_element_per_page;
	uint32_t 			i;
	int32_t 			result = -1;
	void* 				element;

	nb_element_per_page = ARRAY_DEFAULT_PAGE_SIZE / array->element_size;

	for (i = min_index; (i < array->nb_element) && (result == -1) && (i < max_index); i++){
		if (_array_get_length(array->pages) * nb_element_per_page <= i){
			element = array->buffer + (i % nb_element_per_page) * array->element_size;
		}
		else{
			page = (struct arrayPage*)_array_get(array->pages, i / nb_element_per_page);
			element = page->buffer + (i % nb_element_per_page) * array->element_size;
		}

		if (compare(element, key) == 0){
			result = i;
		}
	}

	return result;
}

int32_t array_search_seq_down(struct array* array, uint32_t min_index, uint32_t max_index, void* key, int32_t(*compare)(void* element, void* key)){
	struct arrayPage* 	page;
	uint32_t 			nb_element_per_page;
	uint32_t 			i;
	int32_t 			result = -1;
	void* 				element;

	nb_element_per_page = ARRAY_DEFAULT_PAGE_SIZE / array->element_size;

	for (i = ((max_index + 1) > array->nb_element) ? array->nb_element : (max_index + 1); (result == -1) && (i > 0) && (i > min_index); i--){
		if (_array_get_length(array->pages) * nb_element_per_page <= (i - 1)){
			element = array->buffer + ((i - 1) % nb_element_per_page) * array->element_size;
		}
		else{
			page = (struct arrayPage*)_array_get(array->pages, (i - 1) / nb_element_per_page);
			element = page->buffer + ((i - 1) % nb_element_per_page) * array->element_size;
		}

		if (compare(element, key) == 0){
			result = (i - 1);
		}
	}

	return result;
}

int32_t array_clone(struct array* array_src, struct array* array_dst){
	uint32_t 			i;
	int32_t 			result = -1;
	struct arrayPage* 	page_src;
	struct arrayPage* 	page_dst;

	array_dst->buffer = (char*)malloc(array_src->nb_allocated_byte);
	if (array_dst->buffer == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		return result;
	}
	memcpy(array_dst->buffer, array_src->buffer, array_src->nb_filled_byte);

	array_dst->nb_allocated_byte 	= array_src->nb_allocated_byte;
	array_dst->nb_filled_byte 		= array_src->nb_filled_byte;
	array_dst->nb_element 			= array_src->nb_element;
	array_dst->element_size 		= array_src->element_size;

	if (_array_clone(&(array_src->pages), &(array_dst->pages))){
		printf("ERROR: in %s, unable to clone _array pages\n", __func__);
		free(array_dst->buffer);
		return result;
	}

	for (i = 0; i < _array_get_length(array_dst->pages); i++){
		page_src = (struct arrayPage*)_array_get(array_src->pages, i);
		page_dst = (struct arrayPage*)_array_get(array_dst->pages, i);

		page_dst->buffer = (char*)malloc(page_src->nb_allocated_byte);
		if (page_dst->buffer == NULL){
			printf("ERROR: in %s, unable to allocate memory for page %u\n", __func__, i);
			return result;
		}
		memcpy(page_dst->buffer, page_src->buffer, page_src->nb_filled_byte);
	}

	result = 0;

	return result;
}

int32_t array_copy(struct array* array_src, struct array* array_dst, uint32_t offset, uint32_t nb_element){
	int32_t 			result = -1;
	uint32_t 			nb_remaining_element;
	char* 				buffer;
	uint32_t 			nb_allocated_element;
	uint32_t 			nb_allocated_byte;
	uint32_t 			nb_copied_byte;
	uint32_t 			nb_copied_element;
	struct arrayPage 	page;
	uint32_t			copy_index_src;
	char* 				copy_ptr_src;
	struct arrayPage* 	page_src;


	if (array_src->element_size != array_dst->element_size){
		printf("ERROR: in %s, copy between arrays of different element size is a dangerous thing -> aborting\n", __func__);
		return result;
	}

	if (array_src->nb_element < offset + nb_element){
		printf("ERROR: in %s, source array does not contain required elements -> aborting\n", __func__);
		return result;
	}

	nb_remaining_element = nb_element;

	while (nb_remaining_element > 0){
		nb_allocated_element = ((ARRAY_DEFAULT_PAGE_SIZE - array_dst->nb_filled_byte) / array_dst->element_size > nb_remaining_element) ? nb_remaining_element : ((ARRAY_DEFAULT_PAGE_SIZE - array_dst->nb_filled_byte) / array_dst->element_size);
		nb_allocated_byte = ((array_dst->nb_filled_byte + nb_allocated_element * array_dst->element_size) / ARRAY_DEFAULT_ALLOC_SIZE + (((array_dst->nb_filled_byte + nb_allocated_element * array_dst->element_size) % ARRAY_DEFAULT_ALLOC_SIZE == 0) ? 0 : 1)) * ARRAY_DEFAULT_ALLOC_SIZE;

		if (nb_allocated_byte > array_dst->nb_allocated_byte){
			buffer = realloc(array_dst->buffer, nb_allocated_byte);
			if (buffer == NULL){
				printf("ERROR: in %s, unable to realloc memory\n", __func__);
				break;
			}

			array_dst->buffer = buffer;
			array_dst->nb_allocated_byte = nb_allocated_byte;
		}

		nb_copied_element = 0;
		while(nb_copied_element < nb_allocated_element){
			copy_index_src = offset + (nb_element - nb_remaining_element) + nb_copied_element;

			if (_array_get_length(array_src->pages) * (ARRAY_DEFAULT_PAGE_SIZE / array_src->element_size) <= copy_index_src){
				copy_ptr_src = array_src->buffer + (copy_index_src % (ARRAY_DEFAULT_PAGE_SIZE / array_src->element_size)) * array_src->element_size;
				nb_copied_byte = (nb_allocated_element - nb_copied_element) * array_src->element_size;
			}
			else{
				page_src = (struct arrayPage*)_array_get(array_src->pages, copy_index_src / (ARRAY_DEFAULT_PAGE_SIZE / array_src->element_size));
				copy_ptr_src = page_src->buffer + (copy_index_src % (ARRAY_DEFAULT_PAGE_SIZE / array_src->element_size)) * array_src->element_size;
				nb_copied_byte = (((nb_allocated_element - nb_copied_element) > ((ARRAY_DEFAULT_PAGE_SIZE / array_src->element_size) - (copy_index_src % (ARRAY_DEFAULT_PAGE_SIZE / array_src->element_size)))) ? ((ARRAY_DEFAULT_PAGE_SIZE / array_src->element_size) - (copy_index_src % (ARRAY_DEFAULT_PAGE_SIZE / array_src->element_size))) : (nb_allocated_element - nb_copied_element)) * array_src->element_size;
			}

			memcpy(array_dst->buffer + array_dst->nb_filled_byte, copy_ptr_src, nb_copied_byte);
			nb_copied_element += nb_copied_byte / array_dst->element_size;
			array_dst->nb_filled_byte += nb_copied_byte;
		}

		array_dst->nb_element += nb_allocated_element;

		if (array_dst->nb_allocated_byte == ARRAY_DEFAULT_PAGE_SIZE){
			page.buffer = array_dst->buffer;
			page.nb_allocated_byte = array_dst->nb_allocated_byte;
			page.nb_filled_byte = array_dst->nb_filled_byte;

			if (_array_add(&(array_dst->pages), &page) < 0){
				printf("ERROR: in %s, unable to archive page\n", __func__);
				break;
			}

			array_dst->nb_allocated_byte = 0;
			array_dst->nb_filled_byte = 0;
			array_dst->buffer = NULL;
		}

		nb_remaining_element -= nb_copied_element;
	}

	result = nb_element - nb_remaining_element;

	return result;
}

void array_empty(struct array* array){
	uint32_t i;
	struct arrayPage* page;

	array->nb_filled_byte = 0;
	array->nb_element = 0;

	for (i = 0; i < _array_get_length(array->pages); i++){
		page = (struct arrayPage*)_array_get(array->pages, i);
		free(page->buffer);
	}

	_array_empty(&(array->pages));
}

void array_clean(struct array* array){
	uint32_t i;
	struct arrayPage* page;

	if (array != NULL){
		if (array->buffer != NULL){
			free(array->buffer);
		}

		for (i = 0; i < _array_get_length(array->pages); i++){
			page = (struct arrayPage*)_array_get(array->pages, i);
			free(page->buffer);
		}

		_array_clean(&(array->pages));
	}
}

void array_delete(struct array* array){
	if (array != NULL){
		array_clean(array);
		free(array);
	}
}