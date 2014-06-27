#ifndef SIGNATUREREADER_H
#define SIGNATUREREADER_H

#include <stdint.h>

#include "codeSignature.h"

void codeSignatureReader_parse(struct codeSignatureCollection* collection, const char* file_name);

#endif