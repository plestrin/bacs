#ifndef REFREADERJSON_H
#define REFREADERJSON_H

#include <stdint.h>

#include <yajl/yajl_parse.h>

#include "array.h"

int32_t refReaderJSON_parse(const char* file_name, struct array* array);

#endif