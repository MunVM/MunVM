#include <mun/array.h>
#include "all.h"

NAME(parallel_move);
INPUT_COUNT(parallel_move, 0x0);
DEFINE(ParallelMove){
    NULL, // set_input_at
    NULL, // compile
    NULL, // get_representation
    NULL, // get_input_representation
    NULL, // make_location_summary
    NULL, // argument_at
    NULL, // successor_at
    NULL, // argument_count
    &parallel_move_input_count, // input_count
    NULL, // successor_count
    NULL, // input_at
    &parallel_move_name, // name
};

bool
parallel_move_instr_is_redundant(parallel_move_instr* moves){
  for(word i = 0; i < moves->moves.size; i++){
    if(!move_operands_is_redundant(moves->moves.data[i])){
      return FALSE;
    }
  }
  return TRUE;
}