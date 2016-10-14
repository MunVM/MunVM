#include <mun/common.h>
#if defined(ARCH_IS_X64)

#include "../all.h"

#define COMPILE(Name) \
  void Name##_compile(instruction* instr, asm_buff* code)

#define SUMMARIZE(Name) \
  location_summary* \
  Name##_make_location_summary(instruction* instr)

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

static void
call_native(mun_native_args args, mun_native_function func){
  func(args);
}

COMPILE(constant){
  location_summary* locs = instr->locations;
  if(loc_is_register(locs->output)){
    asm_movq_ri(code, loc_get_register(locs->output), ((asm_imm) to_constant_instr(instr)->value));
  }
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

#endif