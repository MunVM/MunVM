#include <mun/array.h>

void
array_init(array* self, word size){
  self->size = 0x0;
  self->asize = size;
  self->data = malloc(sizeof(V) * size);
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