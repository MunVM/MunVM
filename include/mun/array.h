#ifndef MUN_ARRAY_H
#define MUN_ARRAY_H

#include "common.h"

HEADER_BEGIN

typedef void* V;

typedef struct{
  word asize;
  word size;
  V* data;
} array;

void array_init(array* self, word size);
void array_resize(array* self, word nasize);
void array_dispose(array* self);
void arraycpy(array* to, array* from, int offset, int length);

#define ARRAY(Name) \
  array_init(&Name, 0xA)

#define ARRAY_CLEAR(Name) \
  Name.size = 0x0

#define ARRAYCPY(to, from) \
  arraycpy(to, from, 0, (from)->size)

MUN_INLINE V
array_last(array* self){
  return self->data[self->size - 1];
}

MUN_INLINE V
array_pop(array* self){
  if(self->size == 0) return NULL;
  V res = self->data[self->size - 1];
  self->size--;
  return res;
}

MUN_INLINE void
array_add(array* self, V value){
  array_resize(self, self->size + 1);
  self->data[self->size - 1] = value;
}

MUN_INLINE void
array_insert(array* self, word idx, V value){
  array_resize(self, self->size + 1);
  for(word i = self->size - 2; i >= idx; i--) self->data[i + 1] = self->data[i];
  self->data[idx] = value;
}

MUN_INLINE void
array_add_all(array* self, array* src){
  for(word i = 0; i < src->size; i++) array_add(self, src->data[i]);
}

MUN_INLINE void
array_truncate(array* self, word size){
  self->size = size;
}

MUN_INLINE void
array_erase(array* self, word index){
  for(word i = index; i < self->size - 1; i++) self->data[i] = self->data[i + 1];
  self->size -= 1;
}

MUN_INLINE bool
array_is_empty(array* self){
  return self == NULL ||
         self->size == 0;
}

HEADER_END

#endif