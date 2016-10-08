#include <mun/common.h>
#if defined(ARCH_IS_X64)

#include <mun/codegen/move_resolver.h>
#include <mun/codegen/compiler.h>
#include "../all.h"

#define CODE \
  &moves->compiler->code

void
move_resolver_emit_move(move_resolver* moves, word index){
  move_operands* move = moves->moves.data[index];

  location source = move->src;
  location dest = move->dest;

  if(loc_is_register(source)){
    if(loc_is_register(dest)){
      asm_movq_rr(CODE, loc_get_register(dest), loc_get_register(source));
    } else{
      asm_address slot;
      loc_encode_stack_address(dest, &slot);
      asm_movq_ar(CODE, &slot, loc_get_register(source));
    }
  } else if(loc_is_stack_slot(source)){
    if(loc_is_register(dest)){
      asm_address slot;
      loc_encode_stack_address(source, &slot);
      asm_movq_ra(CODE, loc_get_register(dest), &slot);
    } else{
      asm_address source_slot;
      loc_encode_stack_address(source, &source_slot);

      asm_address dest_slot;
      loc_encode_stack_address(dest, &dest_slot);

      asm_movq_ra(CODE, TMP, &source_slot);
      asm_movq_ar(CODE, &dest_slot, TMP);
    }
  } else if(loc_is_fpu_register(source)){
    if(loc_is_fpu_register(dest)){

    } else{
      asm_address slot;
      loc_encode_stack_address(dest, &slot);

      asm_movsd_ar(CODE, &slot, loc_get_fpu_register(source));
    }
  } else if(loc_is_double_stack_slot(source)){
    if(loc_is_fpu_register(dest)){
      asm_address slot;
      loc_encode_stack_address(source, &slot);

      asm_movsd_ra(CODE, loc_get_fpu_register(dest), &slot);
    } else{
      asm_address source_slot;
      loc_encode_stack_address(source, &source_slot);

      asm_address dest_slot;
      loc_encode_stack_address(dest, &dest_slot);

      asm_movsd_ra(CODE, XMM0, &source_slot);
      asm_movsd_ar(CODE, &dest_slot, XMM0);
    }
  } else{
    instance* constant = loc_get_constant(source)->value;
    if(loc_is_register(dest)){
      asm_movq_ri(CODE, loc_get_register(dest), ((uword) constant));
    } else{
      asm_address slot;
      loc_encode_stack_address(dest, &slot);
      asm_movq_ai(CODE, &slot, ((uword) constant));
    }
  }

  move_operands_eliminate(move);
}

void
move_resolver_emit_swap(move_resolver* moves, word index){
  move_operands* move = moves->moves.data[index];

  location source = move->src;
  location dest = move->dest;

  if(loc_is_register(source) && loc_is_register(dest)){
    asm_movq_rr(CODE, TMP, loc_get_register(source));
    asm_movq_rr(CODE, loc_get_register(source), loc_get_register(dest));
    asm_movq_rr(CODE, loc_get_register(dest), TMP);
  } else{
    fprintf(stderr, "Unreachable\n");
    abort();
  }

  move_operands_eliminate(move);

  for(word i = 0; i < moves->moves.size; i++){
    move_operands* other = moves->moves.data[i];
    if(move_operands_blocks(other, source)){
      ((move_operands*) moves->moves.data[i])->src = dest;
    } else if(move_operands_blocks(other, dest)){
      ((move_operands*) moves->moves.data[i])->dest = source;
    }
  }
}

#endif