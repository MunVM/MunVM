#ifndef MUN_ASM_H
#define MUN_ASM_H

#include "common.h"

HEADER_BEGIN

typedef struct{
  uword contents;
  uword cursor;
  uword limit;
} asm_buff;

#define EMIT(Type) \
  MUN_INLINE void \
  asm_emit_##Type(asm_buff* self, Type value){ \
    *((Type*) self->cursor) = value; \
    self->cursor += sizeof(Type); \
  }
EMIT(uint8_t);
EMIT(int32_t);
EMIT(int64_t);
#undef EMIT

#define IO(Type) \
  MUN_INLINE Type \
  asm_load_##Type(asm_buff* self, word pos){ \
    return ((Type) (self->contents + pos)); \
  } \
  MUN_INLINE void \
  asm_store_##Type(asm_buff* self, word pos, Type value){ \
    *((Type*) (self->contents + pos)) = value; \
  }
IO(int32_t);
#undef IO

void asm_init(asm_buff* self);

MUN_INLINE word
asm_size(asm_buff* self){
  return self->cursor - self->contents;
}

#if defined(ARCH_IS_X64)
#include "x64/core.h"
#else
#error "Unknown CPU architecture"
#endif

HEADER_END

#endif