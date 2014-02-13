#ifndef CMREADERJSON_H
#define CMREADERJSON_H

#include <yajl/yajl_parse.h>

#include "codeMap.h"

struct codeMap* cmReaderJSON_parse(const char* file_name);

#endif