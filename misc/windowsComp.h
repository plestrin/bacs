#ifndef WINDOWSCOMP_H
#define WINDOWSCOMP_H

#ifndef __func__
#	define __func__ __FUNCTION__
#endif

#ifndef snprintf
#	define snprintf(str, size, format, ...) _snprintf_s((str), (size), _TRUNCATE, (format), __VA_ARGS__)
#endif

#ifndef strncpy
#	define strncpy(dst, src, size) strncpy_s((dst), (size), (src), _TRUNCATE)
#endif

#ifndef mkdir
#	define mkdir(name, perm) CreateDirectoryA((name), NULL)
#endif

#ifndef inline
# 	define inline __inline
#endif

char* windowsComp_sanitize_path(char* path);

#endif