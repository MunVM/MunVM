#ifndef MUN_SSTREAM_H
#define MUN_SSTREAM_H

#include "common.h"

HEADER_BEGIN

#ifndef SSTREAM_INIT_SIZE
#define SSTREAM_INIT_SIZE 1024
#endif

#ifndef SSTREAM_GROW_SIZE
#define SSTREAM_GROW_SIZE 16
#endif

typedef struct{
  uint8_t* data;

  size_t size;
  size_t asize;
} sstream;

sstream* sstream_new();

char* sstream_cstr(sstream* self);

char sstream_getc(sstream* self, size_t index);

void sstream_init(sstream* stream);
void sstream_dispose(sstream* stream);
void sstream_free(sstream* self);
void sstream_grow(sstream* self, size_t nasize);
void sstream_put(sstream* self, char* data, size_t size);
void sstream_putc(sstream* self, char data);

MUN_INLINE void
sstream_puts(sstream* self, sstream* other){
  sstream_put(self, ((char*) other->data), other->size);
}

MUN_INLINE void
sstream_putstr(sstream* self, char* str){
  sstream_put(self, str, strlen(str));
}

HEADER_END

#endif