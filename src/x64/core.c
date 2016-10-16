#include <mun/asm.h>

#if defined(TARGET_IS_LINUX)
#include <sys/mman.h>
#endif

MUN_INLINE void
emit_register_rex(asm_buff* self, asm_register reg, uint8_t rex) {
  rex |= (reg > 7) ? REX_B : REX_NONE;
  if (rex != REX_NONE) asm_emit_uint8_t(self, REX_PREFIX | rex);
}

MUN_INLINE void
emit_oper_rex(asm_buff* self, int rm, asm_address* oper, uint8_t rex) {
  rex |= (rm > 7) ? REX_R : REX_NONE;
  if (rex != REX_NONE) asm_emit_uint8_t(self, REX_PREFIX | rex);
}

MUN_INLINE void
set_mod_rm(asm_address* self, int mod, asm_register rm) {
  if ((rm > 7) && !((rm == R12) && (mod != 3))) self->rex |= REX_B;
  self->encoding[0] = ((uint8_t) ((mod << 6) | (rm & 7)));
  self->length = 1;
}

MUN_INLINE void
set_sib(asm_address* self, scale_factor scale, asm_register index, asm_register base) {
  if (base > 7) self->rex |= REX_B;
  if (index > 7) self->rex |= REX_W;
  self->encoding[1] = ((scale << 6) | ((index & 7) << 3) | (base & 7));
  self->length = 2;
}

MUN_INLINE void
set_disp8(asm_address* self, uint8_t disp) {
  self->encoding[self->length++] = disp;
}

MUN_INLINE void
set_disp32(asm_address* self, int32_t disp) {
  memmove(&self->encoding[self->length], &disp, sizeof(disp));
  self->length += sizeof(disp);
}

MUN_INLINE bool
is_register(asm_address* self, asm_register reg) {
  return ((reg > 7 ? 1 : 0) == (self->rex == REX_B)) &&
         ((self->encoding[0] & 0xF8) == 0xC0) &&
         ((self->encoding[0] & 0x07) == reg);
}

MUN_INLINE bool
is8(asm_imm imm){
  uint8_t limit = ((uint8_t) (1)) << 7;
  return (-limit <= imm) && (imm < limit);
}

MUN_INLINE bool
is32(asm_imm imm){
  int32_t limit = ((int32_t) (1)) << 31;
  return (-limit <= imm) && (imm < limit);
}

MUN_INLINE void
emit_immediate(asm_buff* self, asm_imm imm){
  if(is32(imm)){
    asm_emit_int32_t(self, ((int32_t) imm));
  } else{
    asm_emit_int64_t(self, imm);
  }
}

MUN_INLINE void
emit_oper(asm_buff* self, int rm, asm_address* addr){
  word len = addr->length;
  asm_emit_uint8_t(self, ((uint8_t) (addr->encoding[0] + (rm << 3))));
  for(word i = 1; i < len; i++) asm_emit_uint8_t(self, addr->encoding[i]);
}

MUN_INLINE void
emit_complex(asm_buff* self, int rm, asm_operand* oper, asm_imm imm){
  if(is8(imm)){
    asm_emit_uint8_t(self, 0x83);
    emit_oper(self, rm, oper);
    asm_emit_uint8_t(self, ((uint8_t) (imm & 0xFF)));
  } else if(is_register(oper, RAX)){
    asm_emit_uint8_t(self, ((uint8_t) (0x05 + (rm << 3))));
    emit_immediate(self, imm);
  } else{
    asm_emit_uint8_t(self, 0x81);
    emit_oper(self, rm, oper);
    emit_immediate(self, imm);
  }
}

MUN_INLINE void
emit_rex_rb_xx(asm_buff* self, asm_fpu_register reg, asm_fpu_register base, uint8_t rex){
  if(reg > 7) rex |= REX_R;
  if(base > 7) rex |= REX_B;
  if(rex != REX_NONE) asm_emit_uint8_t(self, REX_PREFIX|rex);
}

MUN_INLINE void
emit_rex_rb_ra(asm_buff* self, asm_fpu_register reg, asm_operand* oper, uint8_t rex){
  if(reg > 7) rex |= REX_R;
  rex |= oper->rex;
  if(rex != REX_NONE) asm_emit_uint8_t(self, REX_PREFIX|rex);
}

MUN_INLINE void
emit_xmm_register_operand(asm_buff* self, int rm, asm_fpu_register xmm){
  asm_operand oper;
  set_mod_rm(&oper, 3, ((asm_register) xmm));
  emit_oper(self, rm, &oper);
}

void
asm_addr_init_r(asm_address* self, asm_register base, asm_imm disp){
  self->rex = REX_NONE;
  if((disp == 0) && ((base & 7) != RBP)){
    set_mod_rm(self, 0, base);
    if((base & 7) == RSP) set_sib(self, TIMES_1, RSP, base);
  } else if(is8(disp)){
    set_mod_rm(self, 1, base);
    if((base & 7) == RSP) set_sib(self, TIMES_1, RSP, base);
    set_disp8(self, ((uint8_t) disp));
  } else{
    set_mod_rm(self, 2, base);
    if((base & 7) == RSP) set_sib(self, TIMES_1, RSP, base);
    set_disp32(self, ((int32_t) disp));
  }
}

void
asm_addr_init_s(asm_address* self, asm_register base, scale_factor scale, asm_imm disp){
  set_mod_rm(self, 0, RSP);
  set_sib(self, scale, base, RBP);
  set_disp32(self, ((int32_t) disp));
}

void
asm_oper_init_r(asm_operand* self, asm_register reg){
  self->rex = REX_NONE;
  set_mod_rm(self, 3, reg);
}

void
asm_addq_rr(asm_buff* self, asm_register dst, asm_register src){
  asm_operand oper;
  asm_oper_init_r(&oper, src);

  emit_oper_rex(self, dst, &oper, REX_W);
  asm_emit_uint8_t(self, 0x03);
  emit_oper(self, dst & 7, &oper);
}

void
asm_addq_ra(asm_buff* self, asm_register dst, asm_address* src){
  emit_oper_rex(self, dst, src, REX_W);
  asm_emit_uint8_t(self, 0x03);
  emit_oper(self, dst & 7, src);
}

void
asm_leaq(asm_buff* code, asm_register dst, asm_address* src){
  emit_oper_rex(code, dst, src, REX_W);
  asm_emit_uint8_t(code, 0x8D);
  emit_oper(code, dst & 7, src);
}

void
asm_addq_ri(asm_buff* self, asm_register dst, asm_imm src){
  if(is32(src)){
    emit_register_rex(self, dst, REX_W);

    asm_operand oper;
    asm_oper_init_r(&oper, dst);

    emit_complex(self, 0, &oper, src);
  } else{
    asm_movq_ri(self, TMP, src);
    asm_addq_rr(self, dst, TMP);
  }
}

void
asm_addq_ai(asm_buff* self, asm_address* dst, asm_imm src){
  if(is32(src)){
    emit_oper_rex(self, 0, dst, REX_W);
    emit_complex(self, 0, dst, src);
  } else{
    asm_movq_ri(self, TMP, src);
    asm_addq_ar(self, dst, TMP);
  }
}

void
asm_addq_ar(asm_buff* self, asm_address* dst, asm_register src){
  emit_oper_rex(self, src, dst, REX_W);
  asm_emit_uint8_t(self, 0x01);
  emit_oper(self, src & 7, dst);
}

void
asm_subsd_rr(asm_buff* self, asm_fpu_register dst, asm_fpu_register src){
  asm_emit_uint8_t(self, 0xF2);
  emit_rex_rb_xx(self, dst, src, REX_NONE);
  asm_emit_uint8_t(self, 0x0F);
  asm_emit_uint8_t(self, 0x5C);
  emit_xmm_register_operand(self, dst & 7, src);
}

void
asm_subq_rr(asm_buff* self, asm_register dst, asm_register src){
  asm_operand oper;
  asm_oper_init_r(&oper, src);

  emit_oper_rex(self, dst, &oper, REX_W);
  asm_emit_uint8_t(self, 0x2B);
  emit_oper(self, dst & 7, &oper);
}

void
asm_subq_ri(asm_buff* self, asm_register dst, asm_imm src){
  if(is32(src)){
    emit_register_rex(self, dst, REX_W);

    asm_operand oper;
    asm_oper_init_r(&oper, dst);

    emit_complex(self, 5, &oper, src);
  } else{
    asm_movq_ri(self, TMP, src);
    asm_subq_rr(self, dst, TMP);
  }
}

void
asm_subq_ra(asm_buff* self, asm_register dst, asm_address* src){
  emit_oper_rex(self, dst, src, REX_W);
  asm_emit_uint8_t(self, 0x2B);
  emit_oper(self, dst & 7, src);
}

void
asm_subq_ar(asm_buff* self, asm_address* dst, asm_register src){
  emit_oper_rex(self, src, dst, REX_W);
  asm_emit_uint8_t(self, 0x29);
  emit_oper(self, src & 7, dst);
}

void
asm_subq_ai(asm_buff* self, asm_address* dst, asm_imm src){
  if(is32(src)){
    emit_oper_rex(self, 0, dst, REX_W);
    emit_complex(self, 5, dst, src);
  } else{
    asm_movq_ri(self, TMP, src);
    asm_subq_ar(self, dst, TMP);
  }
}

void
asm_push_i(asm_buff* self, asm_imm src){
  if(is8(src)){
    asm_emit_uint8_t(self, 0x6A);
    asm_emit_uint8_t(self, ((uint8_t) (src & 0xFF)));
  } else if(is32(src)){
    asm_emit_uint8_t(self, 0x68);
    emit_immediate(self, src);
  } else{
    asm_movq_ri(self, TMP, src);
    asm_push_r(self, TMP);
  }
}

void
asm_mulsd_rr(asm_buff* self, asm_fpu_register dst, asm_fpu_register src){
  asm_emit_uint8_t(self, 0xF2);
  emit_rex_rb_xx(self, dst, src, REX_NONE);
  asm_emit_uint8_t(self, 0x0F);
  asm_emit_uint8_t(self, 0x59);
  emit_xmm_register_operand(self, dst & 7, src);
}

void
asm_mulq_rr(asm_buff* self, asm_register dst, asm_register src){
  asm_operand oper;
  asm_oper_init_r(&oper, src);

  emit_oper_rex(self, dst, &oper, REX_W);
  asm_emit_uint8_t(self, 0x0F);
  asm_emit_uint8_t(self, 0xAF);
  emit_oper(self, dst & 7, &oper);
}

void
asm_mulq_ri(asm_buff* self, asm_register dst, asm_imm src){
  if(is32(src)){
    asm_operand oper;
    asm_oper_init_r(&oper, dst);

    emit_oper_rex(self, dst, &oper, REX_W);
    asm_emit_uint8_t(self, 0x69);
    emit_oper(self, dst & 7, &oper);
    emit_immediate(self, src);
  } else{
    asm_movq_ri(self, TMP, src);
    asm_mulq_rr(self, dst, TMP);
  }
}

void
asm_divsd_rr(asm_buff* self, asm_fpu_register dst, asm_fpu_register src){
  asm_emit_uint8_t(self, 0xF2);
  emit_rex_rb_xx(self, dst, src, REX_NONE);
  asm_emit_uint8_t(self, 0x0F);
  asm_emit_uint8_t(self, 0x5E);
  emit_xmm_register_operand(self, dst & 7, src);
}

void
asm_push_r(asm_buff* self, asm_register src){
  emit_register_rex(self, src, REX_NONE);
  asm_emit_uint8_t(self, ((uint8_t) (0x50 | (src & 7))));
}

void
asm_pop_r(asm_buff* self, asm_register dst){
  emit_register_rex(self, dst, REX_NONE);
  asm_emit_uint8_t(self, ((uint8_t) (0x58 | (dst & 7))));
}

void
asm_pop_a(asm_buff* self, asm_address* dst){
  emit_oper_rex(self, 0, dst, REX_NONE);
  asm_emit_uint8_t(self, 0x8F);
  emit_oper(self, 0, dst);
}

void
asm_ret(asm_buff* self){
  asm_emit_uint8_t(self, 0xC3);
}

void
asm_enter_frame(asm_buff* self, word size){
  asm_push_r(self, RBP);
  asm_movq_rr(self, RBP, RSP);
  if(size != 0) asm_subq_ri(self, RSP, size);
}

void
asm_leave_frame(asm_buff* self){
  asm_movq_rr(self, RSP, RBP);
  asm_pop_r(self, RBP);
}

void
asm_jmp_r(asm_buff* self, asm_register dst){
  asm_operand oper;
  asm_oper_init_r(&oper, dst);

  emit_oper_rex(self, 4, &oper, REX_NONE);
  asm_emit_uint8_t(self, 0xFF);
  emit_oper(self, 4, &oper);
}

void
asm_jmp_a(asm_buff* self, asm_address* dst){
  emit_oper_rex(self, 4, dst, REX_NONE);
  asm_emit_uint8_t(self, 0xFF);
  emit_oper(self, 4, dst);
}

void
asm_testq_rr(asm_buff* self, asm_register r1, asm_register r2){
  asm_operand oper;
  asm_oper_init_r(&oper, r2);

  emit_oper_rex(self, r1, &oper, REX_W);
  asm_emit_uint8_t(self, 0x85);
  emit_oper(self, r1 & 7, &oper);
}

void
asm_andq_rr(asm_buff* self, asm_register dst, asm_register src){
  asm_operand oper;
  asm_oper_init_r(&oper, src);
  emit_oper_rex(self, dst, &oper, REX_W);
  asm_emit_uint8_t(self, 0x23);
  emit_oper(self, dst & 7, &oper);
}

void
asm_andq_ri(asm_buff* self, asm_register dst, asm_imm src){
  if(is32(src)){
    emit_register_rex(self, dst, REX_W);
    asm_operand oper;
    asm_oper_init_r(&oper, dst);
    emit_complex(self, 4, &oper, src);
  } else{
    asm_movq_ri(self, TMP, src);
    asm_andq_rr(self, dst, TMP);
  }
}

void
asm_addsd_rr(asm_buff* self, asm_fpu_register dst, asm_fpu_register src){
  asm_emit_uint8_t(self, 0xF2);
  emit_rex_rb_xx(self, src, dst, REX_NONE);
  asm_emit_uint8_t(self, 0x0F);
  asm_emit_uint8_t(self, 0x58);
  emit_xmm_register_operand(self, dst & 7, src);
}

void
asm_push_a(asm_buff* self, asm_address* src){
  emit_oper_rex(self, 6, src, REX_NONE);
  asm_emit_uint8_t(self, 0xFF);
  emit_oper(self, 6, src);
}

void
asm_cmpq_rr(asm_buff* self, asm_register r1, asm_register r2){
  asm_operand oper;
  asm_oper_init_r(&oper, r2);

  emit_oper_rex(self, r1, &oper, REX_W);
  asm_emit_uint8_t(self, 0x3B);
  emit_oper(self, r1 & 7, &oper);
}

void
asm_cmpq_ri(asm_buff* self, asm_register r1, asm_imm i1){
  if(is32(i1)){
    emit_register_rex(self, r1, REX_W);

    asm_operand oper;
    asm_oper_init_r(&oper, r1);

    emit_complex(self, 7, &oper, i1);
  } else{
    asm_movq_ri(self, TMP, i1);
    asm_cmpq_rr(self, r1, TMP);
  }
}

void
asm_movsd_ar(asm_buff* self, asm_address* dst, asm_fpu_register src){
  asm_emit_uint8_t(self, 0xF2);
  emit_rex_rb_ra(self, src, dst, REX_NONE);
  asm_emit_uint8_t(self, 0x0F);
  asm_emit_uint8_t(self, 0x11);
  emit_oper(self, src & 7, dst);
}

void
asm_movsd_ra(asm_buff* self, asm_fpu_register dst, asm_address* src){
  asm_emit_uint8_t(self, 0xF2);
  emit_rex_rb_ra(self, dst, src, REX_NONE);
  asm_emit_uint8_t(self, 0x0F);
  asm_emit_uint8_t(self, 0x10);
  emit_oper(self, dst & 7, src);
}

void
asm_movq_rr(asm_buff* self, asm_register dst, asm_register src){
  asm_operand oper;
  asm_oper_init_r(&oper, dst);

  emit_oper_rex(self, src, &oper, REX_W);
  asm_emit_uint8_t(self, 0x89);
  emit_oper(self, src & 7, &oper);
}

void
asm_movq_ra(asm_buff* self, asm_register dst, asm_address* src){
  emit_oper_rex(self, dst, src, REX_W);
  asm_emit_uint8_t(self, 0x8B);
  emit_oper(self, dst & 7, src);
}

void
asm_call_r(asm_buff* self, asm_register reg){
  asm_operand oper;
  asm_oper_init_r(&oper, reg);

  emit_oper_rex(self, 2, &oper, REX_NONE);
  asm_emit_uint8_t(self, 0xFF);
  emit_oper(self, 2, &oper);
}

void
asm_movq_ri(asm_buff* self, asm_register dst, asm_imm src){
  if(is32(src)){
    asm_operand oper;
    asm_oper_init_r(&oper, dst);

    emit_oper_rex(self, 0, &oper, REX_W);
    asm_emit_uint8_t(self, 0xC7);
    emit_oper(self, 0, &oper);
  } else{
    emit_register_rex(self, dst, REX_W);
    asm_emit_uint8_t(self, ((uint8_t) (0xB8 | (dst & 7))));
  }

  emit_immediate(self, src);
}

void
asm_movq_ar(asm_buff* self, asm_address* dst, asm_register src){
  emit_oper_rex(self, src, dst, REX_W);
  asm_emit_uint8_t(self, 0x89);
  emit_oper(self, src & 7, dst);
}

void
asm_movq_ai(asm_buff* self, asm_address* dst, asm_imm src){
  if(is32(src)){
    emit_oper_rex(self, 0, dst, REX_W);
    asm_emit_uint8_t(self, 0xC7);
    emit_oper(self, 0, dst);
    emit_immediate(self, src);
  } else{
    asm_movq_ri(self, TMP, src);
    asm_movq_ar(self, dst, TMP);
  }
}

#undef MAP_FAILED
#define MAP_FAILED ((void*) (-1))

void*
asm_compile(asm_buff* self){
  if(asm_size(self) <= 0){
    fprintf(stderr, "Cannot finalize code:\n\tSize <= 0\n");
    abort();
  }

  void* chunk;
#if defined(TARGET_IS_LINUX)
  chunk = mmap(0, ((size_t) asm_size(self)), PROT_WRITE|PROT_EXEC, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
  if(chunk == MAP_FAILED){
    char buffer[256];
    strerror_r(errno, buffer, 256);
    fprintf(stderr, "Cannot finalize code:\n\t%s\n", buffer);
    abort();
  }
#elif defined(TARGET_IS_WINDOWS)
  chunk = VirtualAlloc(NULL, size, MEM_RESERVE, PAGE_EXECUTE_READWRITE);
#else
#error "Unknown Operating System"
#endif
  memcpy(chunk, ((void*) self->contents), ((size_t) asm_size(self)));
  return chunk;
}