#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../TEA.h"

int main(){
	char* 			data	= NULL;
	uint32_t		size 	= 32;
	uint32_t		key[4]	 = {0x1245F06A, 0x4589FE60, 0x50AA7859, 0xF56941BB};
	
	data = (char*)malloc(size);
	if (data != NULL){
		memset(data, 0, size);
		strcpy(data, "Hello World!");
		printf("Message 1:\t%s\n", data);
		
		tea_encypher((uint32_t*)data, (uint64_t)(size >> 3), key);
		tea_decipher((uint32_t*)data, (uint64_t)(size >> 3), key);
		
		printf("Message 2:\t%s\n", data);
		
		free(data);
	}
	else{
		printf("ERROR: unable to allocate memory\n");
	}
	
	return 0;
}