#include <mun/codegen/liveness.h>
#include "all.h"

static bool
update_live_out(liveness_analysis* analysis, block_entry_instr* block){
  bit_vector* live_out = analysis->live_out.data[block->postorder_num];
  bool changed = FALSE;

  instruction* last = block->last;
  for(word i = 0; i < instr_successor_count(last); i++){
    block_entry_instr* successor = instr_successor_at(last, i);
    if(bit_vector_add_all(live_out, analysis->live_in.data[successor->postorder_num])) changed = TRUE;
  }

  return changed;
}

static bool
update_live_in(liveness_analysis* analysis, block_entry_instr* block){
  bit_vector* live_out = liveness_get_live_out(analysis, block);
  bit_vector* live_in =liveness_get_live_in(analysis, block);
  bit_vector* kill = liveness_get_kill(analysis, block);
  return bit_vector_kill_and_add(live_in, kill, live_out);
}

static void
liveness_compute_live_sets(liveness_analysis* analysis){
  word block_count = analysis->postorder->size;

  bool changed;
  do{
    changed = FALSE;

    for(word i = 0; i < block_count; i++){
      block_entry_instr* block = analysis->postorder->data[i];
      if(update_live_out(analysis, block) && update_live_in(analysis, block)){
        changed = TRUE;
      }
    }
  } while(changed);
}

void
liveness_analyze(liveness_analysis* analysis){
  for(word i = 0; i < analysis->postorder->size; i++){
    array_add(&analysis->live_out, bit_vector_new(analysis->num_of_vars));
    array_add(&analysis->kill, bit_vector_new(analysis->num_of_vars));
    array_add(&analysis->live_in, bit_vector_new(analysis->num_of_vars));
  }

  analysis->compute_initial_sets(analysis);
  liveness_compute_live_sets(analysis);
}

static void
variable_liveness_compute_initial_sets(liveness_analysis* analysis){
  variable_liveness* vars = container_of(analysis, variable_liveness, analysis);

  bit_vector* loads = malloc(sizeof(bit_vector));
  bit_vector_init(loads, analysis->num_of_vars);

  word block_count = analysis->postorder->size;
  for(word i = 0; i < block_count; i++){
    block_entry_instr* block = analysis->postorder->data[i];

    bit_vector* kill = analysis->kill.data[i];
    bit_vector* live_in = analysis->live_in.data[i];
    bit_vector_clear(loads);

    BACKWARD_ITER(block){
      if(it->type == kLoadLocalInstr){
        load_local_instr* load = to_load_local_instr(it);
        word index = local_var_bit_index(load->local, ((int) vars->num_of_params));
        if(index >= analysis->live_in.size) continue;
        array_add(&analysis->live_in, wdup(index));
        if(!bit_vector_contains(loads, index)){
          bit_vector_add(loads, index);
          load->is_last = TRUE;
        }
        continue;
      }

      if(it->type == kStoreLocalInstr){
        store_local_instr* store = to_store_local_instr(it);
        word index = local_var_bit_index(store->local, ((int) vars->num_of_params));
        if(index >= analysis->live_in.size) continue;
        if(bit_vector_contains(kill, index)){
          if(!bit_vector_contains(live_in, index)) store->is_dead = TRUE;
        } else{
          if(!bit_vector_contains(live_in, index)) store->is_last = TRUE;
          bit_vector_add(kill, index);
        }
        bit_vector_remove(live_in, index);
        continue;
      }
    }
  }
}

array*
variable_liveness_compute_assigned(variable_liveness* analysis){
  ARRAY_CLEAR(analysis->assigned);

  word block_count = analysis->graph->preorder.size;
  for(word i = 0; i < block_count; i++){
    block_entry_instr* block = analysis->graph->preorder.data[i];

    bit_vector* kill = liveness_get_kill(((liveness_analysis*) analysis), block);
    bit_vector_intersect(kill, liveness_get_live_out(((liveness_analysis*) analysis), block));
    array_add(&analysis->assigned, kill);
  }

  return &analysis->assigned;
}

void
variable_liveness_init(variable_liveness* analysis, flow_graph* graph){
  liveness_init(((liveness_analysis*) analysis), graph_variable_count(graph), &graph->postorder);
  analysis->graph = graph;
  analysis->num_of_params = func_num_non_copied_params(graph->func);
  ((liveness_analysis*) analysis)->compute_initial_sets = &variable_liveness_compute_initial_sets;
  ARRAY(analysis->assigned);
}

bool
variable_liveness_is_store_alive(variable_liveness* analysis, block_entry_instr* block, store_local_instr* store){
  if(store->is_dead) return FALSE;
  if(store->is_last) {
    word index = local_var_bit_index(store->local, ((int) analysis->num_of_params));
    return bit_vector_contains(liveness_get_live_out(((liveness_analysis*) analysis), block), index);
  }
  return TRUE;
}

bool
variable_liveness_is_last_load(variable_liveness* analysis, block_entry_instr* block, load_local_instr* load){
  word index = local_var_bit_index(load->local, ((int) analysis->num_of_params));
  return load->is_last &&
         !bit_vector_contains(liveness_get_live_in(((liveness_analysis*) analysis), block), index);
}