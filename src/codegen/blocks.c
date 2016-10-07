#include "all.h"

PREDECESSOR_COUNT(graph_entry, 0);
SUCCESSOR_COUNT(graph_entry, 1);
SUCCESSOR_AT(graph_entry){
  if(index == 0) return ((block_entry_instr*) to_graph_entry_instr(instr)->normal_entry);
  return NULL;
}
NAME(graph_entry);
DEFINE_BLOCK(GraphEntry){
    &graph_entry_predecessor_count, // predecessor_count
    NULL, // add_predecessor
    NULL, // clear_predecessors
    NULL, // predecessor_at
};
DEFINE(GraphEntry){
    NULL, // set_input_at
    NULL, // compile
    NULL, // get_representation
    NULL, // get_input_representation
    NULL, // make_location_summary
    NULL, // argument_at
    &graph_entry_successor_at, // successor_at
    NULL, // argument_count
    NULL, // input_count
    &graph_entry_successor_count, // successor_count
    NULL, // input_at
    &graph_entry_name, // name
};

instruction*
graph_entry_instr_normalize(instruction* instr){
  graph_entry_instr* gentry = to_graph_entry_instr(instr);
  return ((instruction*) gentry->normal_entry);
}

PREDECESSOR_COUNT(target_entry, 1);
PREDECESSOR_AT(target_entry, predecessor);
ADD_PREDECESSOR(target_entry){
  container_of(block, target_entry_instr, block)->predecessor = pred;
}
CLEAR_PREDECESSORS(target_entry){
  container_of(block, target_entry_instr, block)->predecessor = NULL;
}
NAME(target_entry);
DEFINE_BLOCK(TargetEntry){
    &target_entry_predecessor_count, // predecessor_count
    &target_entry_add_predecessor, // add_predecessor
    &target_entry_clear_predecessors, // clear_predecessors
    &target_entry_predecessor_at, // predecessor_at
};
DEFINE(TargetEntry){
    NULL, // set_input_at
    NULL, // compile
    NULL, // get_representation
    NULL, // get_input_representation
    NULL, // make_location_summary
    NULL, // argument_at
    NULL, // successor_at
    NULL, // argument_count
    NULL, // input_count
    NULL, // successor_count
    NULL, // input_at
    &target_entry_name, // name
};