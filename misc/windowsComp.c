#include "windowsComp.h"

char* windowsComp_sanitize_path(char* path){
	unsigned int i = 0;

	while(path[i] != '\0'){
		if (path[i] == '\\'){
			path[i] = '/';
		}
		i++;
	}

	return path;
}