#include <mun/array.h>

void
array_init(array* self, word size){
  self->size = 0x0;
  self->asize = size;
  self->data = malloc(sizeof(V) * size);
  for(int i = 0; i < size; i++) self->data[i] = NULL;
}

void
array_resize(array* self, word nasize){
  if(nasize > self->asize){
    V* ndata = realloc(self->data, sizeof(V) * nasize);
    self->data = ndata;
    self->asize = nasize;
  }
  self->size = nasize;
}

void
array_dispose(array* self){
  if(!self) return;
  free(self->data);
  self->size = self->asize = 0x0;
}

void
arraycpy(array* to, array* from, int offset, int size){
  if(offset > size) return;
  if((sizeof(from->data) / sizeof(V)) < size) return;
  to->size = size;
  to->asize = size;
  if(to->data == NULL) to->data = malloc(sizeof(V) * size);
  if((sizeof(to->data) / sizeof(V)) > size) to->data = realloc(to->data, sizeof(V) * size);
  memcpy(to->data, &from->data[offset], ((size_t) size));
}