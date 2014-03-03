#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "windowsComp.h"

char* windowsComp_sanitize_path(char* path){
	uint32_t i = 0;

	while(path[i] != '\0'){
		if (path[i] == '\\'){
			path[i] = '/';
		}
		i++;
	}

	return path;
}