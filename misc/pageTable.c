#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "pageTable.h"

struct pageTable* pageTable_create(size_t data_size){
	struct pageTable* pt;

	pt = (struct pageTable*)malloc(sizeof(struct pageTable));
	if (pt != NULL){
		if (pageTable_init(pt, data_size)){
			printf("ERROR: in %s, unable to init pageTable\n", __func__);
			free(pt);
			pt = NULL;
		}
	}
	else{
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
	}

	return pt;
}

int32_t pageTable_init(struct pageTable* pt, size_t data_size){
	uint16_t i;

	if (data_size < 1){
		printf("ERROR: in %s, incorrect data size\n", __func__);
		return -1;
	}

	pt->data_size = data_size;

	#ifdef PAGETABLE_TRACK_MEMORY_CONSUMPTION
	pt->memory_consumption = sizeof(struct pageTable);
	#endif

	for (i = 0; i < 256; i++){
		pt->main_pages[i] = NULL;
	}

	return 0;
}

#define PAGETABLE_GET_INDEX1(index) 				((index) >> 24)
#define PAGETABLE_GET_INDEX2(index) 				(((index) >> 16) & 0x000000ff)
#define PAGETABLE_GET_INDEX3(index) 				(((index) >> 8) & 0x000000ff)
#define PAGETABLE_GET_INDEX4(index) 				((index) & 0x000000ff)

#define PAGETABLE_SET_VALUE(index, set_vector) 		((set_vector)[(index) >> 5] |=  0x00000001 << ((index) & 0x0000001f))
#define PAGETABLE_CLEAN_VALUE(index, set_vector) 	((set_vector)[(index) >> 5] &=  ~(0x00000001 << ((index) & 0x0000001f)))
#define PAGETABLE_IS_VALUE_SET(index, set_vector) 	(((set_vector)[(index) >> 5] >> ((index) & 0x0000001f)) & 0x00000001)

int32_t pageTable_add_element(struct pageTable* pt, uint32_t index, void* data){
	struct pageIndex* 	sub_page1;
	struct pageIndex* 	sub_page2;
	struct pageData* 	sub_page3;

	uint8_t 			index1 = PAGETABLE_GET_INDEX1(index);
	uint8_t 			index2 = PAGETABLE_GET_INDEX2(index);
	uint8_t 			index3 = PAGETABLE_GET_INDEX3(index);
	uint8_t 			index4 = PAGETABLE_GET_INDEX4(index);

	if (pt->main_pages[index1] == NULL){
		pt->main_pages[index1] = (struct pageIndex*)malloc(sizeof(struct pageIndex));
		if (pt->main_pages[index1] == NULL){
			printf("ERROR: in %s, unable to allocate memory\n", __func__);
			return -1;
		}
		else{
			sub_page1 = pt->main_pages[index1];
			sub_page1->nb_element = 0;
			memset(sub_page1->pages, 0, 256*sizeof(void*));

			#ifdef PAGETABLE_TRACK_MEMORY_CONSUMPTION
			pt->memory_consumption += sizeof(struct pageIndex);
			#endif
		}	
	}
	else{
		sub_page1 = pt->main_pages[index1];
	}

	if (sub_page1->pages[index2] == NULL){
		sub_page1->pages[index2] = malloc(sizeof(struct pageIndex));
		if (sub_page1->pages[index2] == NULL){
			printf("ERROR: in %s, unable to allocate memory\n", __func__);
			return -1;
		}
		else{
			sub_page1->nb_element ++;
			sub_page2 = (struct pageIndex*)sub_page1->pages[index2];
			sub_page2->nb_element = 0;
			memset(sub_page2->pages, 0, 256*sizeof(void*));

			#ifdef PAGETABLE_TRACK_MEMORY_CONSUMPTION
			pt->memory_consumption += sizeof(struct pageIndex);
			#endif
		}	
	}
	else{
		sub_page2 = (struct pageIndex*)sub_page1->pages[index2];
	}

	if (sub_page2->pages[index3] == NULL){
		sub_page2->pages[index3] = (struct pageData*)malloc(sizeof(struct pageData) + 256 * (pt->data_size - 1));
		if (sub_page2->pages[index3] == NULL){
			printf("ERROR: in %s, unable to allocate memory\n", __func__);
			return -1;
		}
		else{
			sub_page2->nb_element ++;
			sub_page3 = (struct pageData*)sub_page2->pages[index3];
			sub_page3->nb_element = 0;
			memset(sub_page3->set_vector, 0, 8*sizeof(uint32_t));

			#ifdef PAGETABLE_TRACK_MEMORY_CONSUMPTION
			pt->memory_consumption += sizeof(struct pageData) + 256 * (pt->data_size - 1);
			#endif
		}	
	}
	else{
		sub_page3 = (struct pageData*)sub_page2->pages[index3];
	}

	memcpy(sub_page3->data + pt->data_size * index4, data, pt->data_size);
	PAGETABLE_SET_VALUE(index4, sub_page3->set_vector);
	sub_page3->nb_element ++;

	return 0;
}

void* pageTable_get_element(struct pageTable* pt, uint32_t index){
	struct pageIndex* 	sub_page1;
	struct pageIndex* 	sub_page2;
	struct pageData* 	sub_page3;

	uint8_t 			index1 = PAGETABLE_GET_INDEX1(index);
	uint8_t 			index2 = PAGETABLE_GET_INDEX2(index);
	uint8_t 			index3 = PAGETABLE_GET_INDEX3(index);
	uint8_t 			index4 = PAGETABLE_GET_INDEX4(index);

	sub_page1 = pt->main_pages[index1];
	if (sub_page1 == NULL){
		return NULL;
	}

	sub_page2 = (struct pageIndex*)sub_page1->pages[index2];
	if (sub_page2 == NULL){
		return NULL;
	}

	sub_page3 = (struct pageData*)sub_page2->pages[index3];
	if (sub_page3 == NULL){
		return NULL;
	}

	if (PAGETABLE_IS_VALUE_SET(index4, sub_page3->set_vector)){
		return sub_page3->data + pt->data_size * index4;
	}
	else{
		return NULL;
	}
}

void* pageTable_get_or_add_element(struct pageTable* pt, uint32_t index, void* data){
	struct pageIndex* 	sub_page1;
	struct pageIndex* 	sub_page2;
	struct pageData* 	sub_page3;

	uint8_t 			index1 = PAGETABLE_GET_INDEX1(index);
	uint8_t 			index2 = PAGETABLE_GET_INDEX2(index);
	uint8_t 			index3 = PAGETABLE_GET_INDEX3(index);
	uint8_t 			index4 = PAGETABLE_GET_INDEX4(index);

	if (pt->main_pages[index1] == NULL){
		pt->main_pages[index1] = (struct pageIndex*)malloc(sizeof(struct pageIndex));
		if (pt->main_pages[index1] == NULL){
			printf("ERROR: in %s, unable to allocate memory\n", __func__);
			return NULL;
		}
		else{
			sub_page1 = pt->main_pages[index1];
			sub_page1->nb_element = 0;
			memset(sub_page1->pages, 0, 256*sizeof(void*));

			#ifdef PAGETABLE_TRACK_MEMORY_CONSUMPTION
			pt->memory_consumption += sizeof(struct pageIndex);
			#endif
		}	
	}
	else{
		sub_page1 = pt->main_pages[index1];
	}

	if (sub_page1->pages[index2] == NULL){
		sub_page1->pages[index2] = malloc(sizeof(struct pageIndex));
		if (sub_page1->pages[index2] == NULL){
			printf("ERROR: in %s, unable to allocate memory\n", __func__);
			return NULL;
		}
		else{
			sub_page1->nb_element ++;
			sub_page2 = (struct pageIndex*)sub_page1->pages[index2];
			sub_page2->nb_element = 0;
			memset(sub_page2->pages, 0, 256*sizeof(void*));

			#ifdef PAGETABLE_TRACK_MEMORY_CONSUMPTION
			pt->memory_consumption += sizeof(struct pageIndex);
			#endif
		}	
	}
	else{
		sub_page2 = (struct pageIndex*)sub_page1->pages[index2];
	}

	if (sub_page2->pages[index3] == NULL){
		sub_page2->pages[index3] = (struct pageData*)malloc(sizeof(struct pageData) + 256 * (pt->data_size - 1));
		if (sub_page2->pages[index3] == NULL){
			printf("ERROR: in %s, unable to allocate memory\n", __func__);
			return NULL;
		}
		else{
			sub_page2->nb_element ++;
			sub_page3 = (struct pageData*)sub_page2->pages[index3];
			sub_page3->nb_element = 0;
			memset(sub_page3->set_vector, 0, 8*sizeof(uint32_t));

			#ifdef PAGETABLE_TRACK_MEMORY_CONSUMPTION
			pt->memory_consumption += sizeof(struct pageData) + 256 * (pt->data_size - 1);
			#endif
		}	
	}
	else{
		sub_page3 = (struct pageData*)sub_page2->pages[index3];
	}

	if (!PAGETABLE_IS_VALUE_SET(index4, sub_page3->set_vector)){
		memcpy(sub_page3->data + pt->data_size * index4, data, pt->data_size);
		PAGETABLE_SET_VALUE(index4, sub_page3->set_vector);
		sub_page3->nb_element ++;
	}
	

	return sub_page3->data + pt->data_size * index4;
}

void* pageTable_get_first(struct pageTable* pt, uint32_t* index){
	struct pageIndex* 	sub_page1;
	struct pageIndex* 	sub_page2;
	struct pageData* 	sub_page3;

	uint8_t 			index1;
	uint8_t 			index2;
	uint8_t 			index3;
	uint8_t 			index4;

	for (index1 = 0; ; index1++){
		if (pt->main_pages[index1] != NULL){
			sub_page1 = pt->main_pages[index1];
			for (index2 = 0; ; index2++){
				if (sub_page1->pages[index2] != NULL){
					sub_page2 = sub_page1->pages[index2];
					for (index3 = 0; ; index3++){
						if (sub_page2->pages[index3] != NULL){
							sub_page3 = sub_page2->pages[index3];
							for (index4 = 0; ; index4++){
								if (PAGETABLE_IS_VALUE_SET(index4, sub_page3->set_vector)){
									*index = (index1 << 24) | (index2 << 16) | (index3 << 8) | index4;

									return sub_page3->data + pt->data_size * index4;
								}
								if (index4 == 255){
									break;
								}
							}
						}
						if (index3 == 255){
							break;
						}
					}
				}
				if (index2 == 255){
					break;
				}
			}
		}
		if (index1 == 255){
			break;
		}
	}

	return NULL;
}

void* pageTable_get_next(struct pageTable* pt, uint32_t* index){
	struct pageIndex* 	sub_page1;
	struct pageIndex* 	sub_page2;
	struct pageData* 	sub_page3;

	uint8_t 			index1;
	uint8_t 			index2;
	uint8_t 			index3;
	uint8_t 			index4;

	if (*index == 0xffffffff){
		return NULL;
	}

	*index = *index + 1;

	index1 = PAGETABLE_GET_INDEX1(*index);
	index2 = PAGETABLE_GET_INDEX2(*index);
	index3 = PAGETABLE_GET_INDEX3(*index);
	index4 = PAGETABLE_GET_INDEX4(*index);

	sub_page1 = pt->main_pages[index1];
	if (sub_page1 == NULL){
		search_index1:

		index2 = 0;
		index3 = 0;
		index4 = 0;
		
		while(sub_page1 == NULL && index1 != 255){
			index1 ++;
			sub_page1 = pt->main_pages[index1];
		}
		if (sub_page1 == NULL){
			return NULL;
		}
	}

	sub_page2 = (struct pageIndex*)sub_page1->pages[index2];
	if (sub_page2 == NULL){
		search_index2:

		index3 = 0;
		index4 = 0;
		
		while(sub_page2 == NULL && index2 != 255){
			index2 ++;
			sub_page2 = (struct pageIndex*)sub_page1->pages[index2];
		}
		if (sub_page2 == NULL){
			sub_page1 = NULL;
			goto search_index1;
		}
	}

	sub_page3 = (struct pageData*)sub_page2->pages[index3];
	if (sub_page3 == NULL){
		search_index3:

		index4 = 0;

		while(sub_page3 == NULL && index3 != 255){
			index3 ++;
			sub_page3 = (struct pageData*)sub_page2->pages[index3];
		}
		if (sub_page3 == NULL){
			sub_page2 = NULL;
			goto search_index2;
		}
	}

	while(!PAGETABLE_IS_VALUE_SET(index4, sub_page3->set_vector) && index4 != 255){
		index4 ++;
	}
	if (!PAGETABLE_IS_VALUE_SET(index4, sub_page3->set_vector)){
		sub_page3 = NULL;
		goto search_index3;
	}
	
	*index = (index1 << 24) | (index2 << 16) | (index3 << 8) | index4;

	return sub_page3->data + pt->data_size * index4;
}

void* pageTable_get_last(struct pageTable* pt, uint32_t* index){
	struct pageIndex* 	sub_page1;
	struct pageIndex* 	sub_page2;
	struct pageData* 	sub_page3;

	uint8_t 			index1;
	uint8_t 			index2;
	uint8_t 			index3;
	uint8_t 			index4;

	for (index1 = 255; ; index1--){
		if (pt->main_pages[index1] != NULL){
			sub_page1 = pt->main_pages[index1];
			for (index2 = 255; ; index2--){
				if (sub_page1->pages[index2] != NULL){
					sub_page2 = sub_page1->pages[index2];
					for (index3 = 255; ; index3--){
						if (sub_page2->pages[index3] != NULL){
							sub_page3 = sub_page2->pages[index3];
							for (index4 = 255; ; index4--){
								if (PAGETABLE_IS_VALUE_SET(index4, sub_page3->set_vector)){
									*index = (index1 << 24) | (index2 << 16) | (index3 << 8) | index4;

									return sub_page3->data + pt->data_size * index4;
								}
								if (index4 == 0){
									break;
								}
							}
						}
						if (index3 == 0){
							break;
						}
					}
				}
				if (index2 == 0){
					break;
				}
			}
		}
		if (index1 == 0){
			break;
		}
	}

	return NULL;
}

void* pageTable_get_prev(struct pageTable* pt, uint32_t* index){
	struct pageIndex* 	sub_page1;
	struct pageIndex* 	sub_page2;
	struct pageData* 	sub_page3;

	uint8_t 			index1;
	uint8_t 			index2;
	uint8_t 			index3;
	uint8_t 			index4;

	if (*index == 0x00000000){
		return NULL;
	}

	*index = *index - 1;

	index1 = PAGETABLE_GET_INDEX1(*index);
	index2 = PAGETABLE_GET_INDEX2(*index);
	index3 = PAGETABLE_GET_INDEX3(*index);
	index4 = PAGETABLE_GET_INDEX4(*index);

	sub_page1 = pt->main_pages[index1];
	if (sub_page1 == NULL){
		search_index1:

		index2 = 255;
		index3 = 255;
		index4 = 255;
		
		while(sub_page1 == NULL && index1 != 0){
			index1 --;
			sub_page1 = pt->main_pages[index1];
		}
		if (sub_page1 == NULL){
			return NULL;
		}
	}

	sub_page2 = (struct pageIndex*)sub_page1->pages[index2];
	if (sub_page2 == NULL){
		search_index2:

		index3 = 255;
		index4 = 255;
		
		while(sub_page2 == NULL && index2 != 0){
			index2 --;
			sub_page2 = (struct pageIndex*)sub_page1->pages[index2];
		}
		if (sub_page2 == NULL){
			sub_page1 = NULL;
			goto search_index1;
		}
	}

	sub_page3 = (struct pageData*)sub_page2->pages[index3];
	if (sub_page3 == NULL){
		search_index3:

		index4 = 255;

		while(sub_page3 == NULL && index3 != 0){
			index3 --;
			sub_page3 = (struct pageData*)sub_page2->pages[index3];
		}
		if (sub_page3 == NULL){
			sub_page2 = NULL;
			goto search_index2;
		}
	}

	while(!PAGETABLE_IS_VALUE_SET(index4, sub_page3->set_vector) && index4 != 0){
		index4 --;
	}
	if (!PAGETABLE_IS_VALUE_SET(index4, sub_page3->set_vector)){
		sub_page3 = NULL;
		goto search_index3;
	}
	
	*index = (index1 << 24) | (index2 << 16) | (index3 << 8) | index4;

	return sub_page3->data + pt->data_size * index4;
}

void pageTable_remove_element(struct pageTable* pt, uint32_t index){
	struct pageIndex* 	sub_page1;
	struct pageIndex* 	sub_page2;
	struct pageData* 	sub_page3;

	uint8_t 			index1 = PAGETABLE_GET_INDEX1(index);
	uint8_t 			index2 = PAGETABLE_GET_INDEX2(index);
	uint8_t 			index3 = PAGETABLE_GET_INDEX3(index);
	uint8_t 			index4 = PAGETABLE_GET_INDEX4(index);

	sub_page1 = pt->main_pages[index1];
	if (sub_page1 != NULL){

		sub_page2 = (struct pageIndex*)sub_page1->pages[index2];
		if (sub_page2 != NULL){
			
			sub_page3 = (struct pageData*)sub_page2->pages[index3];
			if (sub_page3 != NULL){
				
				if (PAGETABLE_IS_VALUE_SET(index4, sub_page3->set_vector)){
					PAGETABLE_CLEAN_VALUE(index4, sub_page3->set_vector);
					sub_page3->nb_element --;

					if (sub_page3->nb_element == 0){
						free(sub_page2->pages[index3]);
						sub_page2->pages[index3] = NULL;
						sub_page2->nb_element --;

						#ifdef PAGETABLE_TRACK_MEMORY_CONSUMPTION
						pt->memory_consumption -= sizeof(struct pageData) + 256 * (pt->data_size - 1);
						#endif

						if (sub_page2->nb_element == 0){
							free(sub_page1->pages[index2]);
							sub_page1->pages[index2] = NULL;
							sub_page1->nb_element --;

							#ifdef PAGETABLE_TRACK_MEMORY_CONSUMPTION
							pt->memory_consumption -= sizeof(struct pageIndex);
							#endif

							if (sub_page1->nb_element == 0){
								free(pt->main_pages[index1]);
								pt->main_pages[index1] = NULL;

								#ifdef PAGETABLE_TRACK_MEMORY_CONSUMPTION
								pt->memory_consumption -= sizeof(struct pageIndex);
								#endif
							}
						}
					}
				}
			}
		}
	}
}

void pageTable_clean(struct pageTable* pt){
	struct pageIndex* 	sub_page1;
	struct pageIndex* 	sub_page2;
	uint16_t 			i1;
	uint16_t 			i2;
	uint16_t 			i3;

	for (i1 = 0; i1 < 256; i1++){
		if (pt->main_pages[i1] != NULL){
			sub_page1 = (struct pageIndex*)pt->main_pages[i1];

			for (i2 = 0; i2 < 256; i2++){
				if (sub_page1->pages[i2] != NULL){
					sub_page2 = (struct pageIndex*)sub_page1->pages[i2];

					for (i3 = 0; i3 < 256; i3++){
						if (sub_page2->pages[i3] != NULL){
							free(sub_page2->pages[i3]);
						}
					}

					free(sub_page1->pages[i2]);
				}
			}

			free(pt->main_pages[i1]);
		}
	}
}

void pageTable_delete(struct pageTable* pt){
	if (pt != NULL){
		pageTable_clean(pt);
		free(pt);
	}
}