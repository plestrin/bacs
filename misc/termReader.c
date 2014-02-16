#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>

#include "termReader.h"

int32_t termReader_set_raw_mode(){
	struct termios settings;

	if (isatty(STDIN_FILENO)){
		if (tcgetattr(STDIN_FILENO, &settings)){
			printf("ERROR: in %s, tcgetattr fails\n", __func__);
			return -1;
		}

		settings.c_lflag &= ~ICANON;

		if (tcsetattr(STDIN_FILENO, TCSANOW, &settings)){
			printf("ERROR: in %s, tcsetattr fails\n", __func__);
			return -1;
		}
	}
	#ifdef VERBOSE
	else{
		printf("WARNING: in %s, stdin is not a terminal - skipping\n", __func__);
	}
	#endif

	return 0;
}

int32_t termReader_get_line(char* buffer, uint32_t buffer_length){
	uint32_t 	i = 0;
	char 		character;

	do{
		character = fgetc(stdin);
		switch(character){
			case '\t' 	: {break;}
			case '\n' 	: {break;}
			case '\b' 	: {break;}
			case EOF	: {
				printf("ERROR: in %s, fgetc returns EOF on stdin\n", __func__);
				return - 1;
			}
			default 	: {
				buffer[i++] = character;
				break;
			}
		}
	} while(character != '\n' && i < buffer_length - 1);
	buffer[i] = '\0';

	return 0;
}