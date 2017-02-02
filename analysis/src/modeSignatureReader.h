#ifndef MODESIGNATUREREADER_H
#define MODESIGNATUREREADER_H

#include <stdint.h>

#include "signatureCollection.h"

void modeSignatureReader_parse(struct signatureCollection* collection, const char* file_name);

#endif
