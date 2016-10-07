#ifndef MUN_GRAPH_H
#define MUN_GRAPH_H

#include "../common.h"

HEADER_BEGIN

#include "../array.h"
#include "../ast.h"
#include "instruction.h"

typedef struct _flow_graph{
  graph_entry_instr* graph_entry;
  word current_ssa_temp_index;
  function* func;
  array parent; // word
  array preorder; // block_entry_instr*
  array postorder; // block_entry_instr*
  array reverse_postorder; // block_entry_instr*
} flow_graph;

MUN_INLINE void
graph_init(flow_graph* graph, function* func, graph_entry_instr* entry){
  graph->current_ssa_temp_index = 0x0;
  graph->func = func;
  graph->graph_entry = entry;
  ARRAY(graph->preorder);
  ARRAY(graph->postorder);
  ARRAY(graph->reverse_postorder);
}

MUN_INLINE word
graph_variable_count(flow_graph* graph){
  return graph->func->num_copied_params +
         func_num_non_copied_params(graph->func) +
         graph->func->num_stack_locals;
}

MUN_INLINE word
graph_alloc_ssa_temp_index(flow_graph* graph){
  return graph->current_ssa_temp_index++;
}

MUN_INLINE void
graph_alloc_ssa_indexes(flow_graph* graph, definition* defn){
  defn->ssa_temp_index = graph_alloc_ssa_temp_index(graph);
  graph_alloc_ssa_temp_index(graph);
}

definition* graph_get_constant(flow_graph* graph, instance* value);

void graph_compute_ssa(flow_graph* graph, word vreg);
void graph_discover_blocks(flow_graph* graph);

typedef struct{
  ast_node* ast;
  function* func;
  graph_entry_instr* entry;
  word temp_count;
} flow_graph_builder;

void graph_builder_init(flow_graph_builder* builder, function* func);

flow_graph* graph_build(flow_graph_builder* builder);

MUN_INLINE word
graph_builder_alloc_temp(flow_graph_builder* builder){
  return builder->temp_count += 1;
}

MUN_INLINE void
graph_builder_dealloc_temps(flow_graph_builder* builder, word count){
  builder->temp_count -= count;
}

typedef struct{
  ast_node_visitor visitor;

  void (*return_definition)(ast_node_visitor*, definition*);
  void (*return_value)(ast_node_visitor*, input*);

  flow_graph_builder* owner;
  instruction* exit;
  instruction* entry;
} effect_visitor;

MUN_INLINE bool
evis_is_empty(effect_visitor* vis){
  return vis->entry == NULL;
}

MUN_INLINE bool
evis_is_open(effect_visitor* vis){
  return evis_is_empty(vis) || vis->exit != NULL;
}

void evis_init(effect_visitor* vis, flow_graph_builder* owner);
void evis_append(effect_visitor* vis, effect_visitor other);
void evis_do(effect_visitor* vis, definition* defn);
void evis_add_return_exit(effect_visitor* vis, input* value);
void evis_add_instruction(effect_visitor* vis, instruction* instr);

input* evis_bind(effect_visitor* vis, definition* defn);

typedef struct{
  effect_visitor visitor;

  input* value;
} value_visitor;

void vvis_init(value_visitor* vis, flow_graph_builder* owner);

#define EVIS(v) container_of(v, effect_visitor, visitor)
#define VVIS(v) container_of(EVIS(v), value_visitor, visitor)

MUN_INLINE void
vis_return_value(ast_node_visitor* vis, input* value){
  EVIS(vis)->return_value(vis, value);
}

MUN_INLINE void
vis_return_definition(ast_node_visitor* vis, definition* defn){
  EVIS(vis)->return_definition(vis, defn);
}

HEADER_END

#endif