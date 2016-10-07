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

COMPILE(constant){
  location_summary* locs = instr->locations;
  if(loc_is_register(locs->output)){
    asm_movq_ri(code, loc_get_register(locs->output), ((asm_imm) to_constant_instr(instr)->value));
  }
}

#endif