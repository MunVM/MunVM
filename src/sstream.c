#include <mun/sstream.h>

sstream*
sstream_new(){
  sstream* stream = malloc(sizeof(sstream));
  if(stream){
    stream->data = malloc(sizeof(char) * SSTREAM_INIT_SIZE);
    stream->size = 0x0;
    stream->asize = SSTREAM_INIT_SIZE;
  }
  return stream;
}

void
sstream_init(sstream* self){
  self->data = malloc(sizeof(char) * SSTREAM_INIT_SIZE);
  self->size = 0x0;
  self->asize = SSTREAM_INIT_SIZE;
}

char*
sstream_cstr(sstream* self){
  if(self->size < self->asize && self->data[self->size] == 0x0) return ((char*) self->data);
  sstream_grow(self, self->size + 1);
  self->data[self->size] = 0x0;
  return ((char*) self->data);
}

char
sstream_getc(sstream* self, size_t index){
  if(index > self->size || index < 0) return 0;
  return ((char) self->data[index]);
}

void
sstream_dispose(sstream* self){
  if(!self) return;
  free(self->data);
}

void
sstream_free(sstream* self){
  if(!self) return;
  free(self->data);
  free(self);
}

void
sstream_grow(sstream* self, size_t nsize){
  if(self->asize > nsize) return;

  size_t nasize = self->asize + SSTREAM_GROW_SIZE;
  while(nasize < nsize) nasize += SSTREAM_GROW_SIZE;

  self->data = realloc(self->data, nasize);
  self->asize = nasize;
}

void
sstream_put(sstream* self, char* data, size_t len){
  if(self->size + len >= self->asize) sstream_grow(self, self->size + len);
  memcpy(self->data + self->size, data, len);
  self->size += len;
}

void
sstream_putc(sstream* self, char data){
  if(self->size + 1 >= self->asize) sstream_grow(self, self->size + 1);
  self->data[self->size++] = ((uint8_t) data);
}