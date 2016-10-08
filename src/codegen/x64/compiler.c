#include <mun/codegen/compiler.h>
#include <mun/location.h>
#include "../all.h"

#define CODE &compiler->code

void
flow_graph_compiler_emit_epilogue(flow_graph_compiler* compiler, instruction* instr){
  if(instr_is_definition(instr)){
    definition* defn = container_of(instr, definition, instr);
    if(defn->temp_index >= 0){
      location value = instr->locations->output;
      if(loc_is_register(value)){
        asm_push_r(CODE, loc_get_register(value));
      } else if(loc_is_constant(value)){
        asm_push_i(CODE, ((uword) loc_get_constant(value)->value));
      } else{
        asm_address slot;
        loc_encode_stack_address(value, &slot);
        asm_push_a(CODE, &slot);
      }
    }
  }
}

void
flow_graph_compiler_emit_prologue(flow_graph_compiler* compiler, instruction* instr){
  // Fallthrough
  //TODO: Something here? Eliminate?
}

MUN_INLINE word
stack_size(flow_graph_compiler* compiler){
  return compiler->func->num_stack_locals +
         compiler->func->num_copied_params;
}

void
flow_graph_compiler_emit_entry(flow_graph_compiler* compiler){
  asm_enter_frame(CODE, stack_size(compiler));
}

void
flow_graph_compiler_compile(flow_graph_compiler* compiler){
  flow_graph_compiler_emit_entry(compiler);

  word num_locals = compiler->func->num_stack_locals;
  if(num_locals > 1) asm_movq_ri(CODE, RAX, ((uword) &NIL));
  word stack_base = compiler->func->first_stack_local_index;

  for(word i = 0; i < num_locals; i++) {
    asm_address slot;
    asm_addr_init_r(&slot, RBP, ((stack_base - i) * kWordSize));
    asm_movq_ar(CODE, &slot, RAX);
  }

  flow_graph_compiler_visit_blocks(compiler);
}