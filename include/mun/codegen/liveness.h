#ifndef MUN_LIVENESS_H
#define MUN_LIVENESS_H

#include <mun/array.h>
#include "../common.h"

HEADER_BEGIN

#include "../bitvec.h"
#include "../array.h"
#include "instruction.h"
#include "graph.h"

typedef struct _liveness_analysis{
  void (*compute_initial_sets)(struct _liveness_analysis*);

  array postorder; // block_entry_instr*
  array live_in; // bit_vector*
  array live_out; // bit_vector*
  array kill; // bit_vector*
  word var_count;
} liveness_analysis;

MUN_INLINE void
liveness_init(liveness_analysis* analysis, word var_count, array* /* block_entry_instr* */ postorder){

  analysis->var_count = var_count;
  array_init(&analysis->live_in, postorder->size);
  array_init(&analysis->live_out, postorder->size);
  array_init(&analysis->kill, postorder->size);
  analysis->postorder = *postorder;
}

#define SET(Name) \
  MUN_INLINE bit_vector* \
  liveness_get_##Name##_at(liveness_analysis* analysis, word postorder_num){ \
    return analysis->Name.data[postorder_num]; \
  } \
  MUN_INLINE bit_vector* \
  liveness_get_##Name(liveness_analysis* analysis, block_entry_instr* block){ \
    return analysis->Name.data[block->postorder_num]; \
  }
SET(live_in);
SET(live_out);
#undef SET

MUN_INLINE bit_vector*
liveness_get_kill(liveness_analysis* analysis, block_entry_instr* block){
  return analysis->kill.data[block->postorder_num];
}

void liveness_compute_live_sets(liveness_analysis* analysis);
void liveness_analyze(liveness_analysis* analysis);

bool liveness_update_live_out(liveness_analysis* analysis, block_entry_instr* block);
bool liveness_update_live_in(liveness_analysis* analysis, block_entry_instr* block);

typedef struct{
  liveness_analysis analysis;

  word params;
  flow_graph* graph;
  array assigned_vars; // bit_vector*
} variable_liveness;

void var_liveness_init(variable_liveness* analysis, flow_graph* graph);

array* var_liveness_compute_assigned_vars(variable_liveness* analysis);

bool var_liveness_is_store_alive(variable_liveness* analysis, block_entry_instr* block, store_local_instr* store);
bool var_liveness_is_last_load(variable_liveness* analysis, block_entry_instr* block, load_local_instr* load);

HEADER_END

#endif