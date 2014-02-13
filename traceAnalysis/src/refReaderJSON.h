#ifndef REFREADERJSON_H
#define REFEADERJSON_H

#include <stdint.h>

#include <yajl/yajl_parse.h>

#include "array.h"
/* other stuff */

int32_t refReaderJSON_parse(const char* file_name, struct array* array);

#endif