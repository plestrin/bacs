#ifndef CMREADERJSON_H
#define CMREADERJSON_H

#include <yajl/yajl_parse.h>

#include "codeMap.h"

struct codeMap* cmReaderJSON_parse(const char* directory_path, uint32_t pid);

#endif
