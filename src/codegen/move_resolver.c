#include <mun/codegen/compiler.h>
#include <mun/codegen/move_resolver.h>
#include <mun/array.h>
#include "all.h"
#include "parallel_move.h"

void
move_resolver_init(move_resolver* moves, flow_graph_compiler* compiler){
  array_init(&moves->moves, 0x4);
  moves->compiler = compiler;
}

static void
build_initial_moves_list(move_resolver* moves, parallel_move_instr* move){
  for(word i = 0; i < move->moves.size; i++){
    move_operands* m = move->moves.data[i];
    if(!move_operands_is_redundant(m)) array_add(&moves->moves, m);
  }
}

static void
perform_move(move_resolver* moves, word index){
  location dest = move_operands_mark_pending(moves->moves.data[index]);

  for(word i = 0; i < moves->moves.size; i++){
    move_operands* other = moves->moves.data[i];
    if(move_operands_blocks(other, dest) &&
       !move_operands_is_pending(other)){
      perform_move(moves, i);
    }
  }

  move_operands_clear_pending(moves->moves.data[index], dest);

  if(((move_operands*) moves->moves.data[index])->src != dest){
    move_operands_eliminate(moves->moves.data[index]);
    return;
  }

  for(word i = 0; i < moves->moves.size; i++){
    move_operands* other = moves->moves.data[i];
    if(move_operands_blocks(other, dest)){
      move_resolver_emit_swap(moves, index);
      return;
    }
  }

  move_resolver_emit_move(moves, index);
}

void
move_resolver_compile(move_resolver* moves, parallel_move_instr* move){
  build_initial_moves_list(moves, move);

  for(word i = 0; i < moves->moves.size; i++){
    move_operands* m = moves->moves.data[i];
    if(!move_operands_is_eliminated(m) && !loc_is_constant(m->src)){

    }
  }
}