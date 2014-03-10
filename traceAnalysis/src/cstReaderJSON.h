#ifndef CSTREADERJSON_H
#define CSTREADERJSON_H

#include <stdint.h>

#include <yajl/yajl_parse.h>

#include "array.h"

int32_t cstReaderJSON_parse(const char* file_name, struct array* array);

#endif