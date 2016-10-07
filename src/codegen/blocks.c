#include <mun/array.h>
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

PREDECESSOR_COUNT(join_entry, container_of(block, join_entry_instr, block)->predecessors.size);
PREDECESSOR_AT(join_entry, predecessors.data[index]);
ADD_PREDECESSOR(join_entry){
  array_add(&container_of(block, join_entry_instr, block)->predecessors, pred);
};
CLEAR_PREDECESSORS(join_entry){
  ARRAY_CLEAR(container_of(block, join_entry_instr, block)->predecessors);
};
NAME(join_entry);
DEFINE_BLOCK(JoinEntry){
    &join_entry_predecessor_count, // predecessor_count
    &join_entry_add_predecessor, // add_predecessor
    &join_entry_clear_predecessors, // clear_predecessors
    &join_entry_predecessor_at, // predecessor_at
};
DEFINE(JoinEntry){
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

phi_instr*
join_entry_instr_insert_phi(join_entry_instr* join, word var_index, word var_count){
  if(join->phis == NULL){
    join->phis = malloc(sizeof(array));
    array_init(join->phis, var_count);
    for(word i = 0; i < var_count; i++) array_add(join->phis, NULL);
  }

  return (join->phis->data[var_index] = phi_instr_new(join, block_predecessor_count(((block_entry_instr*) join))));
}