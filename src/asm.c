#include <mun/asm.h>

static const word kMinGap = 32;
static const word kInitialBufferCapacity = 4 * (1024 * 1024);

void
asm_init(asm_buff* self){
  self->cursor = self->contents = ((uword) (malloc(kInitialBufferCapacity)));
  self->limit = self->contents + kInitialBufferCapacity + kMinGap;
}