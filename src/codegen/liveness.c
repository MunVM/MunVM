#include <mun/codegen/liveness.h>
#include <mun/array.h>
#include "all.h"

bool
liveness_update_live_in(liveness_analysis* analysis, block_entry_instr* block){
  bit_vector* live_out = analysis->live_out.data[block->postorder_num];
  bit_vector* live_in = analysis->live_in.data[block->postorder_num];
  bit_vector* kill = analysis->kill.data[block->postorder_num];
  return bit_vector_kill_and_add(live_in, kill, live_out);
}

bool
liveness_update_live_out(liveness_analysis* analysis, block_entry_instr* block){
  bit_vector* live_out = analysis->live_out.data[block->postorder_num];

  bool changed = FALSE;

  instruction* last = block->last;
  for(int i = 0; i < instr_successor_count(last); i++){
    block_entry_instr* successor = instr_successor_at(last, i);
    changed = bit_vector_add_all(live_out, analysis->live_in.data[block->postorder_num]);
  }

  return changed;
}

void
liveness_compute_live_sets(liveness_analysis* analysis){
  bool changed;
  do{
    changed = FALSE;

    for(int i = 0; i < analysis->postorder.size; i++){
      block_entry_instr* block = analysis->postorder.data[i];
      changed = (liveness_update_live_out(analysis, block) && liveness_update_live_in(analysis, block));
    }
  } while(changed);
}

void
liveness_analyze(liveness_analysis* analysis){
  for(int i = 0; i < analysis->postorder.size; i++){
    array_add(&analysis->live_out, bit_vector_new(analysis->var_count));
    array_add(&analysis->kill, bit_vector_new(analysis->var_count));
    array_add(&analysis->live_in, bit_vector_new(analysis->var_count));
  }

  analysis->compute_initial_sets(analysis);
  liveness_compute_live_sets(analysis);
}

static void
var_liveness_compute_initial_sets(liveness_analysis* analysis){
  variable_liveness* vars = container_of(analysis, variable_liveness, analysis);

  bit_vector* last_loads = bit_vector_new(analysis->var_count);
  for(int i = 0; i < analysis->postorder.size; i++){
    block_entry_instr* block = analysis->postorder.data[i];

    bit_vector* kill = analysis->kill.data[i];
    bit_vector* live_in = analysis->live_in.data[i];
    bit_vector_clear(last_loads);

    BACKWARD_ITER(block){
      if(it->type == kLoadLocalInstr){
        load_local_instr* load = to_load_local_instr(it);
        word index = local_var_bit_index(load->local, ((int) vars->params));
        if(index >= live_in->size) continue;
        bit_vector_add(live_in, index);
        if(!bit_vector_contains(last_loads, index)){
          bit_vector_add(last_loads, index);
          load->is_last = TRUE;
        }
      } else if(it->type == kStoreLocalInstr){
        store_local_instr* store = to_store_local_instr(it);
        word index = local_var_bit_index(store->local, ((int) vars->params));
        if(index >= live_in->size) continue;
        if(bit_vector_contains(kill, index)){
          if(!bit_vector_contains(live_in, index)){
            store->is_dead = TRUE;
          }
        } else{
          if(!bit_vector_contains(live_in, index)){
            store->is_last = TRUE;
          }
          bit_vector_add(kill, index);
        }
        bit_vector_remove(live_in, index);
      }
    }
  }
}

array*
var_liveness_compute_assigned_vars(variable_liveness* analysis){
  ARRAY_CLEAR(analysis->assigned_vars);

  for(word i = 0; i < analysis->graph->preorder.size; i++){
    block_entry_instr* block = analysis->graph->preorder.data[i];

    bit_vector* kill = liveness_get_kill(((liveness_analysis*) analysis), block);
    bit_vector_intersect(kill, liveness_get_live_out(((liveness_analysis*) analysis), block));
    array_add(&analysis->assigned_vars, kill);
  }

  return &analysis->assigned_vars;
}

bool
var_liveness_is_store_alive(variable_liveness* analysis, block_entry_instr* block, store_local_instr* store){
  if(store->is_dead) return FALSE;
  if(store->is_last){
    word index = local_var_bit_index(store->local, ((int) analysis->params));
    return bit_vector_contains(liveness_get_live_out(((liveness_analysis*) analysis), block), index);
  }
  return TRUE;
}

bool
var_liveness_is_last_load(variable_liveness* analysis, block_entry_instr* block, load_local_instr* load){
  word index = local_var_bit_index(load->local, ((int) analysis->params));
  return load->is_last &&
         !bit_vector_contains(liveness_get_live_in(((liveness_analysis*) analysis), block), index);
}

void
var_liveness_init(variable_liveness* analysis, flow_graph* graph){
  liveness_init(((liveness_analysis*) analysis), graph_variable_count(graph), &graph->postorder);
  analysis->graph = graph;
  analysis->params = func_num_non_copied_params(graph->func);
  analysis->analysis.compute_initial_sets = &var_liveness_compute_initial_sets;
  ARRAY(analysis->assigned_vars);
}

static void
ssa_liveness_compute_initial_sets(liveness_analysis* analysis){
  ssa_liveness* ssa = container_of(analysis, ssa_liveness, analysis);

  word block_count = analysis->postorder.size;
  for(word i = 0; i < block_count; i++){
    block_entry_instr* block = analysis->postorder.data[i];

    bit_vector* kill = analysis->kill.data[i];
    bit_vector* live_in = analysis->live_in.data[i];

    BACKWARD_ITER(block){
      instr_initialize_location_summary(it);
      location_summary* locs = it->locations;

      if(instr_is_definition(it)){
        definition* defn = container_of(it, definition, instr);
        if(defn->temp_index >= 0){
          bit_vector_add(kill, defn->ssa_temp_index);
          bit_vector_remove(live_in, defn->ssa_temp_index);
        }
      }

      for(word j = 0; j < instr_input_count(it); j++){
        if(loc_is_constant(locs->inputs[j])) continue;
        input* in = instr_input_at(it, j);
        bit_vector_add(live_in, in->defn->ssa_temp_index);
      }
    }

    if(block->type == kJoinEntryBlock){
      join_entry_instr* join = container_of(block, join_entry_instr, block);
      //TODO: Analyze Phis
    }
  }

  for(word i = 0; i < ssa->gentry->definitions.size; i++){
    definition* defn = ssa->gentry->definitions.data[i];
    word vreg = defn->ssa_temp_index;
    bit_vector_add(analysis->kill.data[((block_entry_instr*) ssa->gentry)->postorder_num], vreg);
    bit_vector_remove(analysis->live_in.data[((block_entry_instr*) ssa->gentry)->postorder_num], vreg);
  }
}

void
ssa_liveness_init(ssa_liveness* analysis, flow_graph* graph){
  liveness_init(((liveness_analysis*) analysis), graph->current_ssa_temp_index, &graph->postorder);
  ((liveness_analysis*) analysis)->compute_initial_sets = &ssa_liveness_compute_initial_sets;
  analysis->gentry = graph->graph_entry;
}