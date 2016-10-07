#ifndef MUN_BITVEC_H
#define MUN_BITVEC_H

#include "common.h"

HEADER_BEGIN

#include "iterator.h"

typedef struct{
  word size;
  word asize;
  uword* data;
} bit_vector;

MUN_INLINE void
bit_vector_remove(bit_vector* vec, word index){
  vec->data[index / kBitsPerWord] &= ~(((uword) 1) << (index % kBitsPerWord));
}

MUN_INLINE void
bit_vector_add(bit_vector* vec, word i){
  vec->data[i / kBitsPerWord] |= (((uword) 1) << (i % kBitsPerWord));
}

MUN_INLINE void
bit_vector_clear(bit_vector* vec){
  for(int i = 0; i < vec->asize; i++) vec->data[i] = 0x0;
}

MUN_INLINE void
bit_vector_init(bit_vector* vec, word size){
  vec->size = size;
  vec->asize= (1 + ((size - 1) / kBitsPerWord));
  vec->data = malloc(sizeof(uword) * vec->asize);
  bit_vector_clear(vec);
}

MUN_INLINE void
bit_vector_set_all(bit_vector* vec){
  for(int i = 0; i < vec->asize; i++) vec->data[i] = ((uword) -1);
}

MUN_INLINE void
bit_vector_intersect(bit_vector* vec, bit_vector* other){
  for(int i = 0; i < vec->asize; i++) vec->data[i] = vec->data[i] & other->data[i];
}

MUN_INLINE bool
bit_vector_contains(bit_vector* vec, word index){
  uword block = vec->data[index / kBitsPerWord];
  return (block & (((uword) 1) << (index % kBitsPerWord))) != 0;
}

MUN_INLINE bool
bit_vector_add_all(bit_vector* vec, bit_vector* from){
  bool changed = FALSE;

  for(int i = 0; i < vec->asize; i++){
    uword before = vec->data[i];
    uword after = vec->data[i] | from->data[i];
    if(before != after){
      changed = TRUE;
      vec->data[i] = after;
    }
  }

  return changed;
}

MUN_INLINE bool
bit_vector_kill_and_add(bit_vector* vec, bit_vector* kill, bit_vector* gen){
  bool changed = FALSE;

  for(int i = 0; i < vec->asize; i++){
    uword before = vec->data[i];
    uword after = vec->data[i] | (gen->data[i] & ~kill->data[i]);
    if(before != after){
      changed = TRUE;
      vec->data[i] = after;
    }
  }

  return changed;
}

MUN_INLINE bit_vector*
bit_vector_new(word size){
  bit_vector* vec = malloc(sizeof(bit_vector));
  bit_vector_init(vec, size);
  return vec;
}

typedef struct{
  iterator iter;

  bit_vector* target;
  word bit_index;
  word word_index;
  uword current_word;
} bit_vector_iterator;

void bit_vector_iterator_init(bit_vector_iterator* iter, bit_vector* owner);

MUN_INLINE bit_vector_iterator*
bit_vector_iterator_new(bit_vector* vec){
  bit_vector_iterator* iter = malloc(sizeof(bit_vector_iterator));
  bit_vector_iterator_init(iter, vec);
  return iter;
}

HEADER_END

#endif