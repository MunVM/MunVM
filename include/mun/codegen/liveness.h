#ifndef MUN_LIVENESS_H
#define MUN_LIVENESS_H

#include <mun/array.h>
#include "../common.h"

HEADER_BEGIN

#include "../array.h"
#include "../bitvec.h"
#include "instruction.h"
#include "flow_graph.h"

typedef struct _liveness_analysis{
  array live_out;
  array live_in;
  array kill;
  array* postorder;
  word num_of_vars;
  void (*compute_initial_sets)(struct _liveness_analysis*);
} liveness_analysis;

MUN_INLINE void
liveness_init(liveness_analysis* analysis, word num_of_vars, array* postorder){
  analysis->num_of_vars = num_of_vars;
  analysis->postorder = postorder;
  array_init(&analysis->live_in, postorder->size);
  array_init(&analysis->live_out, postorder->size);
  array_init(&analysis->kill, postorder->size);
}

#define GET(Name) \
  MUN_INLINE bit_vector* \
  liveness_get_##Name##_at(liveness_analysis* analysis, word postorder){ \
    return (analysis)->Name.data[postorder]; \
  } \
  MUN_INLINE bit_vector* \
  liveness_get_##Name(liveness_analysis* analysis, block_entry_instr* block){ \
    return (analysis)->Name.data[block->postorder_num]; \
  }
GET(live_in);
GET(live_out);
GET(kill);
#undef GET

void liveness_analyze(liveness_analysis* analysis);

typedef struct{
  liveness_analysis analysis;

  word num_of_params;
  flow_graph* graph;
  array assigned; // bit_vector*
} variable_liveness;

void variable_liveness_init(variable_liveness* analysis, flow_graph* graph);

array* variable_liveness_compute_assigned(variable_liveness* analysis);

bool variable_liveness_is_store_alive(variable_liveness* analysis, block_entry_instr* block, store_local_instr* store);
bool variable_liveness_is_last_load(variable_liveness* analysis, block_entry_instr* block, load_local_instr* load);

HEADER_END

#endif