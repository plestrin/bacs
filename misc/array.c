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

int _array_add(struct _array* _array, void* element){
	int 		result = -1;
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

int array_init(struct array* array, uint32_t element_size){
	int result = -1;

	if (array != NULL){
		_array_init(&(array->pages), sizeof(struct arrayPage));

		array->buffer = (char*)malloc(ARRAY_DEFAULT_ALLOC_SIZE);
		if (array->buffer != NULL){
			array->nb_allocated_byte = ARRAY_DEFAULT_ALLOC_SIZE;
			array->nb_filled_byte = 0;
			array->nb_element = 0;
			array->element_size = element_size;

			if (array->element_size > ARRAY_DEFAULT_PAGE_SIZE){
				printf("WARNING: in %s, element size is larger than a page\n", __func__);
			}
		}
		else{
			printf("ERROR: in %s, unable to allocate memory\n", __func__);
			_array_clean(&(array->pages));
			result = -1;
		}
	}

	return result;
}

int array_add(struct array* array, void* element){
	int 				result = -1;
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
		result = array->nb_filled_byte / array->element_size;
		array->nb_filled_byte += array->element_size;
		array->nb_element ++;
	}

	return result;
}

void* array_get(struct array* array, uint32_t index){
	struct arrayPage* 	page;
	uint32_t 			nb_element_per_page;

	nb_element_per_page = ARRAY_DEFAULT_PAGE_SIZE / array->element_size;
	if (_array_get_length(array->pages) * nb_element_per_page < index){
		return array->buffer + (index % nb_element_per_page) * array->element_size;
	}
	else{
		page = (struct arrayPage*)_array_get(array->pages, index / nb_element_per_page);
		return page->buffer + (index % nb_element_per_page) * array->element_size;
	}
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