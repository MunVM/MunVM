#include <mun/bitvec.h>

static void
bit_vector_iterator_advance(iterator* iter){
  bit_vector_iterator* it = container_of(iter, bit_vector_iterator, iter);
  it->bit_index++;
  if(it->current_word == 0){
    do{
      it->word_index++;
      if(it->word_index >= it->target->asize) return;
      it->current_word = it->target->data[it->word_index];
    } while(it->current_word == 0);
    it->bit_index = it->word_index * kBitsPerWord;
  }

  while((it->current_word & 0xFF) == 0){
    it->current_word >>= 8;
    it->bit_index += 8;
  }

  while((it->current_word & 0x1) == 0){
    it->current_word >>= 1;
    it->bit_index++;
  }

  it->current_word = it->current_word >> 1;
}

static void*
bit_vector_iterator_current(iterator* iter){
  return ((void*) container_of(iter, bit_vector_iterator, iter)->bit_index);
}

static bool
bit_vector_iterator_done(iterator* iter){
  bit_vector_iterator* it = container_of(iter, bit_vector_iterator, iter);
  return it->word_index >= it->target->asize;
}

void
bit_vector_iterator_init(bit_vector_iterator* iter, bit_vector* vec){
  iter->target = vec;
  iter->bit_index = -1;
  iter->word_index = 0;
  iter->current_word = vec->data[0x0];
  iter->iter.advance = &bit_vector_iterator_advance;
  iter->iter.current = &bit_vector_iterator_current;
  iter->iter.done = &bit_vector_iterator_done;
  bit_vector_iterator_advance(((iterator*) iter));
}