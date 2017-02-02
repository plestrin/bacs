#ifndef CODESIGNATUREREADER_H
#define CODESIGNATUREREADER_H

#include <stdint.h>

#include "signatureCollection.h"

void codeSignatureReader_parse(struct signatureCollection* collection, const char* file_name);

#endif
