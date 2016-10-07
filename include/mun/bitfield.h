#ifndef MUN_BITFIELD_H
#define MUN_BITFIELD_H

#include "common.h"

HEADER_BEGIN

static const uword kUWordOne = 1U;

typedef struct{
  int pos;
  int size;
} bit_field;

MUN_INLINE word
bit_field_encode(const bit_field* field, word value){
  return (value << field->pos);
}

MUN_INLINE word
bit_field_decode(const bit_field* field, word value){
  return ((value >> field->pos) & ((kUWordOne << field->size) - 1));
}

HEADER_END

#endif