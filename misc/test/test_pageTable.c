#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "../pageTable.h"
#include "../base.h"

#define NB_ELEMENT 		12
#define INDEX_TO_REMOVE 5

int main(){
	struct pageTable* 	pt;
	uint32_t 			i;
	uint32_t 			elements_address[NB_ELEMENT] = {0x00000012, 0x00000056, 0xffffffff, 0x12345678, 0x74561232, 0x45567898, 0x00000000, 0x00004565, 0x12650000, 0x012305604, 0x00369874, 0xab4562f1};
	uint32_t 			elements_value[NB_ELEMENT] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
	uint32_t 			read_address;
	uint32_t* 			read_value;

	pt = pageTable_create(sizeof(uint32_t));
	if (pt == NULL){
		log_err("unable to create pageTable");
	}

	log_info_m("trying to add %u element(s) to the page table", NB_ELEMENT);

	for (i = 0; i < NB_ELEMENT; i++){
		if (pageTable_add_element(pt, elements_address[i], elements_value + i)){
			log_err("unable to add element in pageTable");
			break;
		}
	}

	read_value = (uint32_t*)pageTable_get_first(pt, &read_address);
	while(read_value != NULL){
		log_info_m("read value: %u \t@ 0x%08x", *read_value, read_address);
		read_value = (uint32_t*)pageTable_get_next(pt, &read_address);
	}
	
	log_info_m("trying to remove element %u", elements_value[INDEX_TO_REMOVE]);

	pageTable_remove_element(pt, elements_address[INDEX_TO_REMOVE]);

	read_value = (uint32_t*)pageTable_get_last(pt, &read_address);
	while(read_value != NULL){
		log_info_m("read value: %u \t@ 0x%08x", *read_value, read_address);
		read_value = (uint32_t*)pageTable_get_prev(pt, &read_address);
	}

	log_info_m("memory consumption: %u byte(s)", pageTable_get_memory_consumption(pt));

	pageTable_delete(pt);

	return 0;
}