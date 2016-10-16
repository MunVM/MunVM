#ifndef MUN_CORE_H
#define MUN_CORE_H

#if !defined(MUN_ASM_H)
#error "Please #include <mun/asm.h> directly"
#endif

#include "../asm.h"

HEADER_BEGIN

typedef enum {
  RAX = 0,
  RCX = 1,
  RDX = 2,
  RBX = 3,
  RSP = 4,
  RBP = 5,
  RSI = 6,
  RDI = 7,
  R8 = 8,
  R9 = 9,
  R10 = 10,
  R11 = 11,
  R12 = 12,
  R13 = 13,
  R14 = 14,
  R15 = 15,
  kNumberOfCpuRegisters = 16,
  kNoRegister = -1
} asm_register;

static const char* asm_registers[] = {
    "rax", "rcx", "rdx",
    "rbx", "rsp", "rbp",
    "rsi", "rdi", "r8",
    "r9", "r10", "r11",
    "r12", "r13", "r14",
    "r15"
};

typedef enum{
  XMM0 = 0,
  XMM1 = 1,
  XMM2 = 2,
  XMM3 = 3,
  XMM4 = 4,
  XMM5 = 5,
  XMM6 = 6,
  XMM7 = 7,
  XMM8 = 8,
  XMM9 = 9,
  XMM10 = 10,
  XMM11 = 11,
  XMM12 = 12,
  XMM13 = 13,
  XMM14 = 14,
  XMM15 = 15,
  kNumberOfFpuRegisters = 16
} asm_fpu_register;

typedef enum{
  REX_NONE = 0,
  REX_B = 1 << 0,
  REX_X = 1 << 1,
  REX_R = 1 << 2,
  REX_W = 1 << 3,
  REX_PREFIX = 1 << 6
} rex_bit;

typedef enum{
  TIMES_1 = 0,
  TIMES_2 = 1,
  TIMES_4 = 2,
  TIMES_8 = 3,
  TIMES_16 = 4
} scale_factor;

typedef enum{
  OVERFLOW = 0,
  NO_OVERFLOW = 1,
  BELOW = 2,
  ABOVE_EQUAL = 3,
  EQUAL = 4,
  NOT_EQUAL = 5,
  BELOW_EQUAL = 6,
  ABOVE = 7,
  SIGN = 8,
  NOT_SIGN = 9,
  PARITY_EVEN = 10,
  PARITY_ODD = 11,
  LESS = 12,
  GREATER_EQUAL = 13,
  LESS_EQUAL = 14,
  GREATER = 15,
  ZERO = EQUAL,
  NOT_ZERO = NOT_EQUAL,
  NEGATIVE = SIGN,
  POSITIVE = NOT_SIGN,
  CARRY = BELOW,
  NOT_CARRY = ABOVE_EQUAL
} condition;

static const asm_register TMP = R11;
static const asm_register CODE_REG = R12;
static const asm_register SPREG = RSP;
static const asm_register FPREG = RBP;

static const asm_fpu_register FPUTMP = XMM0;

typedef int64_t asm_imm;

typedef struct{
  uint8_t length;
  uint8_t rex;
  uint8_t encoding[6];
} asm_operand;

void asm_oper_init_r(asm_operand* self, asm_register reg);

typedef asm_operand asm_address;

void asm_addr_init_r(asm_address* self, asm_register base, asm_imm disp);
void asm_addr_init_s(asm_address* self, asm_register base, scale_factor scale, asm_imm disp);

void* asm_compile(asm_buff* self);

// addq
void asm_addq_rr(asm_buff* self, asm_register dst, asm_register src);
void asm_addq_ri(asm_buff* self, asm_register dst, asm_imm src);
void asm_addq_ra(asm_buff* self, asm_register dst, asm_address* src);
void asm_addq_ar(asm_buff* self, asm_address* dst, asm_register src);
void asm_addq_ai(asm_buff* self, asm_address* dst, asm_imm src);

// addsd
void asm_addsd_rr(asm_buff* self, asm_fpu_register dst, asm_fpu_register src);

// subq
void asm_subq_rr(asm_buff* self, asm_register dst, asm_register src);
void asm_subq_ri(asm_buff* self, asm_register dst, asm_imm src);
void asm_subq_ra(asm_buff* self, asm_register dst, asm_address* src);
void asm_subq_ar(asm_buff* self, asm_address* dst, asm_register src);
void asm_subq_ai(asm_buff* self, asm_address* dst, asm_imm src);

// subsd
void asm_subsd_rr(asm_buff* self, asm_fpu_register dst, asm_fpu_register src);

// mulq
void asm_mulq_rr(asm_buff* self, asm_register dst, asm_register src);
void asm_mulq_ri(asm_buff* self, asm_register dst, asm_imm src);

// mulsd
void asm_mulsd_rr(asm_buff* self, asm_fpu_register dst, asm_fpu_register src);

// divsd
void asm_divsd_rr(asm_buff* self, asm_fpu_register dst, asm_fpu_register src);

// movq
void asm_movq_rr(asm_buff* self, asm_register dst, asm_register src);
void asm_movq_ri(asm_buff* self, asm_register dst, asm_imm src);
void asm_movq_ra(asm_buff* self, asm_register dst, asm_address* src);
void asm_movq_ar(asm_buff* self, asm_address* dst, asm_register src);
void asm_movq_ai(asm_buff* self, asm_address* dst, asm_imm src);

// movsd
void asm_movsd_ra(asm_buff* self, asm_fpu_register dst, asm_address* src);
void asm_movsd_ar(asm_buff* self, asm_address* dst, asm_fpu_register src);

// push
void asm_push_r(asm_buff* self, asm_register src);
void asm_push_i(asm_buff* self, asm_imm src);
void asm_push_a(asm_buff* self, asm_address* src);

// pop
void asm_pop_r(asm_buff* self, asm_register dst);
void asm_pop_a(asm_buff* self, asm_address* dst);

// jmp
void asm_jmp_r(asm_buff* self, asm_register dst);
void asm_jmp_a(asm_buff* self, asm_address* dst);

// cmpq
void asm_cmpq_rr(asm_buff* self, asm_register r1, asm_register r2);
void asm_cmpq_ri(asm_buff* self, asm_register r1, asm_imm imm);

// ret
void asm_ret(asm_buff* self);

// leaq
void asm_leaq(asm_buff* self, asm_register dst, asm_address* src);

// call
void asm_call_r(asm_buff* self, asm_register r);

// andq
void asm_andq_rr(asm_buff* self, asm_register dst, asm_register src);
void asm_andq_ri(asm_buff* self, asm_register dst, asm_imm src);

// testq
void asm_testq_rr(asm_buff* self, asm_register r1, asm_register r2);

// Helpers
void asm_enter_frame(asm_buff* self, word frame_size);
void asm_leave_frame(asm_buff* self);

#define ADDRESS(Name, Register, Offset) \
  asm_address Name##_addr; \
  asm_addr_init_r(&Name##_addr, Register, Offset);

HEADER_END

#endif