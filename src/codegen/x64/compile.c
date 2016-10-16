#include <mun/common.h>
#include <mun/type.h>
#include <assert.h>

#if defined(ARCH_IS_X64)

#include "../all.h"

#define COMPILE(Name) \
  void Name##_compile(instruction* instr, asm_buff* code)

#define SUMMARIZE(Name) \
  location_summary* \
  Name##_make_location_summary(instruction* instr)

SUMMARIZE(store_local){
  location_summary* locs = loc_summary_new(1, 0, NO_CALL);
  loc_init_r(&locs->inputs[0], RDI);
  return locs;
}

COMPILE(store_local){
  asm_register store = loc_get_register(instr->locations->inputs[0]);
}

SUMMARIZE(return){
  location_summary* locs = loc_summary_new(1, 0, NO_CALL);
  loc_init_r(&locs->inputs[0], RAX);
  return locs;
}

COMPILE(return){
  asm_ret(code);
}

SUMMARIZE(constant){
  location out;
  loc_init_r(&out, RAX);
  return loc_summary_make(0, out, NO_CALL);
}

COMPILE(constant){
  location_summary* locs = instr->locations;
  printf("Constant %f := #%s\n", to_constant_instr(instr)->value->as.number, asm_registers[loc_get_register(locs->output)]);

  if(loc_is_register(locs->output)){
    asm_movq_ri(code, loc_get_register(locs->output), ((asm_imm) to_constant_instr(instr)->value));
  }
}

static void
call_native(mun_native_args args, mun_native_function func){
  func(args);
}

COMPILE(native_call){
  location_summary* locs = instr->locations;
  function* func = to_native_call_instr(instr)->func;

  asm_push_i(code, ((uword) &NIL));

  ADDRESS(params, RBP, (kParamEndSlotFromFp + func->num_params) * kWordSize);
  asm_leaq(code, RAX, &params_addr);
  asm_movq_ri(code, R10, func->num_params);
  asm_movq_ri(code, RBX, ((uword) func->native));
  asm_subq_ri(code, RSP, sizeof(native_arguments));
  asm_andq_ri(code, RSP, ((asm_imm) ~(15)));

  ADDRESS(argc, RSP, ARGC_OFFSET);
  asm_movq_ar(code, &argc_addr, R10);

  ADDRESS(argv, RSP, ARGV_OFFSET);
  asm_movq_ar(code, &argv_addr, RAX);

  ADDRESS(locals, RBP, 2 * kWordSize);
  asm_movq_ra(code, RAX, &locals_addr);

  ADDRESS(retval, RSP, RETVAL_OFFSET);
  asm_movq_ar(code, &retval_addr, RAX);

  asm_movq_rr(code, RDI, RSP);
  asm_movq_ri(code, RSI, ((uword) func->native));
  asm_movq_ri(code, RAX, ((uword) &call_native));
  asm_call_r(code, RAX);

  asm_pop_r(code, loc_get_register(locs->output));
}

SUMMARIZE(binary_op){
  location_summary* locs = loc_summary_new(2, 0, NO_CALL);
  loc_init_r(&locs->inputs[0], RAX);
  loc_init_r(&locs->inputs[1], RBX);
  loc_init_r(&locs->output, RAX);
  return locs;
}

COMPILE(binary_op){
  location_summary* locs = instr->locations;

  asm_register r1 = loc_get_register(locs->inputs[0]);
  asm_register r2 = loc_get_register(locs->inputs[1]);
  asm_register r3 = loc_get_register(locs->output);

  asm_address r1_val_addr;
  asm_addr_init_r(&r1_val_addr, r1, offsetof(instance, as.number));

  asm_address r2_val_addr;
  asm_addr_init_r(&r2_val_addr, r2, offsetof(instance, as.number));

  asm_movsd_ra(code, XMM0, &r1_val_addr);
  asm_movsd_ra(code, XMM1, &r2_val_addr);
  switch(to_binary_op_instr(instr)->operation){
    case kAdd: asm_addsd_rr(code, XMM0, XMM1); break;
    case kSubtract: asm_subsd_rr(code, XMM0, XMM1); break;
    case kMultiply: asm_mulsd_rr(code, XMM0, XMM1); break;
    case kDivide: asm_divsd_rr(code, XMM0, XMM1); break;
    default:{
      fprintf(stderr, "Unreachable\n");
      abort();
    }
  }

  asm_address r3_val_addr;
  asm_addr_init_r(&r3_val_addr, r3, offsetof(instance, as.number));

  asm_movq_ri(code, r3, ((asm_imm) number_new(GC, 0.0)));
  asm_movsd_ar(code, &r3_val_addr, XMM0);
}

#endif