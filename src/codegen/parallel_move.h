#ifndef MUN_PARALLEL_MOVE_H
#define MUN_PARALLEL_MOVE_H

#if !defined(MUN_ALL_H)
#error "Please #include <mun/all.h> directly"
#endif

typedef struct{
  location dest;
  location src;
} move_operands;

MUN_INLINE void
move_operands_init(move_operands* moveops, location dest, location src){
  moveops->dest = dest;
  moveops->src = src;
}

MUN_INLINE bool
move_operands_is_eliminated(move_operands* moveops){
  return loc_is_invalid(moveops->src);
}

MUN_INLINE bool
move_operands_is_redundant(move_operands* moveops){
  return move_operands_is_eliminated(moveops) ||
         loc_is_invalid(moveops->dest) ||
         moveops->src == moveops->dest;
}

MUN_INLINE bool
move_operands_blocks(move_operands* moveops, location loc){
  return !move_operands_is_eliminated(moveops) &&
         moveops->src == loc;
}

MUN_INLINE bool
move_operands_is_pending(move_operands* moveops){
  return loc_is_invalid(moveops->dest) &&
         !loc_is_invalid(moveops->src);
}

MUN_INLINE void
move_operands_clear_pending(move_operands* moveops, location dest){
  moveops->dest = dest;
}

MUN_INLINE void
move_operands_eliminate(move_operands* moveops){
  moveops->src = moveops->dest = kInvalidLocation;
}

MUN_INLINE location
move_operands_mark_pending(move_operands* moveops){
  location dest = moveops->dest;
  moveops->dest = kInvalidLocation;
  return dest;
}

#define DESTINATION(moveops) &(moveops)->dest
#define SOURCE(moveops) &(moveops)->src

struct _parallel_move_instr{
  instruction instr;

  array moves; // move_operands*
};

#endif