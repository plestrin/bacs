#ifndef BASE_H
#define BASE_H

#include <stdio.h>

#ifdef COLOR

#define ANSI_COLOR_BOLD 		"\x1b[1m"
#define ANSI_COLOR_RED 			"\x1b[31m"
#define ANSI_COLOR_GREEN 		"\x1b[32m"
#define ANSI_COLOR_YELLOW 		"\x1b[33m"
#define ANSI_COLOR_BLUE 		"\x1b[34m"
#define ANSI_COLOR_MAGENTA 		"\x1b[35m"
#define ANSI_COLOR_CYAN 		"\x1b[36m"
#define ANSI_COLOR_BOLD_RED 	"\x1b[1;31m"
#define ANSI_COLOR_BOLD_GREEN 	"\x1b[1;32m"
#define ANSI_COLOR_BOLD_YELLOW 	"\x1b[1;33m"
#define ANSI_COLOR_RESET 		"\x1b[0m"

#else

#define ANSI_COLOR_BOLD
#define ANSI_COLOR_RED
#define ANSI_COLOR_GREEN
#define ANSI_COLOR_YELLOW
#define ANSI_COLOR_BLUE
#define ANSI_COLOR_MAGENTA
#define ANSI_COLOR_CYAN
#define ANSI_COLOR_BOLD_RED
#define ANSI_COLOR_BOLD_GREEN
#define ANSI_COLOR_BOLD_YELLOW
#define ANSI_COLOR_RESET

#endif

#define log_err(M) fprintf(stderr, ANSI_COLOR_RED "[ERROR]" ANSI_COLOR_RESET " (%s:%d) " M "\n", __FILE__, __LINE__)

#define log_warn(M) fprintf(stdout, ANSI_COLOR_YELLOW "[WARN]" ANSI_COLOR_RESET " (%s:%d) " M "\n", __FILE__, __LINE__)

#define log_info(M) fprintf(stdout, ANSI_COLOR_BOLD "[INFO]" ANSI_COLOR_RESET " (%s:%d) " M "\n", __FILE__, __LINE__)

#define log_debug(M) fprintf(stdout, ANSI_COLOR_BLUE "[DEBUG]" ANSI_COLOR_RESET " (%s:%d) " M "\n", __FILE__, __LINE__)

#define log_err_m(M, ...) fprintf(stderr, ANSI_COLOR_RED "[ERROR]" ANSI_COLOR_RESET " (%s:%d) " M "\n", __FILE__, __LINE__, __VA_ARGS__)

#define log_warn_m(M, ...) fprintf(stdout, ANSI_COLOR_YELLOW "[WARN]" ANSI_COLOR_RESET " (%s:%d) " M "\n", __FILE__, __LINE__, __VA_ARGS__)

#define log_info_m(M, ...) fprintf(stdout, ANSI_COLOR_BOLD "[INFO]" ANSI_COLOR_RESET " (%s:%d) " M "\n", __FILE__, __LINE__, __VA_ARGS__)

#define log_debug_m(M, ...) fprintf(stdout, ANSI_COLOR_BLUE "[DEBUG]" ANSI_COLOR_RESET " (%s:%d) " M "\n", __FILE__, __LINE__, __VA_ARGS__)

#ifndef min
#define min(a, b) (((a) > (b)) ? (b) : (a))
#endif
#define max(a, b) (((a) > (b)) ? (a) : (b))
#define abs(a) (((a) ^ ((a) >> (sizeof(a) * 8 - 1))) - ((a) >> (sizeof(a) * 8 - 1)))

enum allocationType{
	ALLOCATION_MALLOC,
	ALLOCATION_MMAP
};

#endif