#include <mun/codegen/flow_graph.h>
#include <mun/bitvec.h>
#include <mun/codegen/liveness.h>
#include "all.h"

definition*
graph_get_constant(flow_graph* graph, instance* value){
  instruction* c = constant_instr_new(value);
  definition* cDef = container_of(c, definition, instr);
  cDef->ssa_temp_index = graph_alloc_ssa_temp_index(graph);
  c->prev = ((instruction*) graph->graph_entry);
  array_add(&graph->graph_entry->definitions, cDef);
  return cDef;
}

typedef struct{
  block_entry_instr* block;
  word next_successor;
} block_traversal_state;

void
graph_discover_blocks(flow_graph* graph){
  ARRAY_CLEAR(graph->preorder);
  ARRAY_CLEAR(graph->postorder);
  ARRAY_CLEAR(graph->reverse_postorder);
  ARRAY_CLEAR(graph->parent);

  array block_stack;
  ARRAY(block_stack);

  block_discover_blocks(((block_entry_instr*) graph->graph_entry), NULL, &graph->preorder, &graph->parent);

  block_traversal_state init_state = {
      ((block_entry_instr*) graph->graph_entry),
      (instr_successor_count(((instruction*) graph->graph_entry)) - 1)
  };
  array_add(&block_stack, &init_state);

  while(!array_is_empty(&block_stack)){
    block_traversal_state* state = array_last(&block_stack);

    if(state->next_successor >= 0){
      block_entry_instr* successor = instr_successor_at(state->block->last, state->next_successor--);
      if(block_discover_blocks(successor, state->block, &graph->preorder, &graph->parent)){
        block_traversal_state new_state = {
            successor,
            (instr_successor_count(successor->last) - 1)
        };
        array_add(&block_stack, &new_state);
      }
    } else{
      array_pop(&block_stack);
      state->block->postorder_num = graph->postorder.size;
      array_add(&graph->postorder, state->block);
    }
  }

  word count = graph->postorder.size;
  for(int i = 0; i < count; i++) array_add(&graph->reverse_postorder, graph->postorder.data[count - i - 1]);
}

MUN_INLINE word
minimum(word a, word b){
  return a < b ? a : b;
}

static void
compress_path(word start_index, word current_index, array* /* word */ parent, array* /* word */ label){
  word next_index = *((word*) parent->data[current_index]);
  if(next_index > start_index){
    compress_path(start_index, next_index, parent, label);
    *((word*) label->data[current_index]) = minimum(*((word*) label->data[current_index]), *((word*) label->data[next_index]));
    parent->data[current_index] = parent->data[next_index];
  }
}

static void
compute_dominators(flow_graph* graph, array* /* bit_vector* */ dominance){
  word size = graph->parent.size;

  array idom; // word
  array_init(&idom, size);
  array semi; // word
  array_init(&semi, size);
  array label; // word
  array_init(&label, size);

  for(int i = 0; i < size; i++){
    array_add(&idom, graph->parent.data[i]);
    array_add(&semi, wdup(i));
    array_add(&label, wdup(i));

    array_add(dominance, bit_vector_new(size));
  }

  ARRAY_CLEAR(((block_entry_instr*) graph->preorder.data[0x0])->dominated);

  for(word block_index = size - 1; block_index >= 1; block_index--){
    block_entry_instr* block = graph->preorder.data[block_index];
    ARRAY_CLEAR(block->dominated);

    for(word i = 0; i < block_predecessor_count(block); i++){
      block_entry_instr* predecessor = block_predecessor_at(block, i);
      word pred_index = predecessor->preorder_num;
      word best = pred_index;
      if(pred_index > block_index){
        compress_path(block_index, pred_index, &graph->parent, &label);
      }

      *((word*) semi.data[block_index]) = minimum(*((word*) semi.data[block_index]), *((word*) semi.data[best]));
    }

    label.data[block_index] = semi.data[block_index];
  }

  for(word block_index = 1; block_index < size; block_index++){
    word dom_index = *((word*) idom.data[block_index]);
    while(dom_index > *((word*) semi.data[block_index])){
      dom_index = *((word*) idom.data[dom_index]);
    }
    *((word*) idom.data[block_index]) = dom_index;
    block_add_dominated(graph->preorder.data[dom_index], graph->preorder.data[block_index]);
  }

  for(word block_index = 0; block_index < size; block_index++){
    block_entry_instr* block = graph->preorder.data[block_index];
    word count = block_predecessor_count(block);
    if(count <= 1) continue;
    for(word i = 0; i < count; i++){
      block_entry_instr* runner = block_predecessor_at(block, i);
      while(runner != block->dominator){
        bit_vector_add(dominance->data[runner->preorder_num], block_index);
        runner = runner->dominator;
      }
    }
  }
}

static void
insert_phis(flow_graph* graph, array* /* block_entry_instr* */ preorder, array* /* bit_vector* */ assigned_vars, array* /* bit_vector* */ dominance, array* /* phi_instr* */ live_phis){
  word block_count = preorder->size;

  array has_already; // word
  array_init(&has_already, block_count);
  array work; // word
  array_init(&work, block_count);

  for(word block_index = 0; block_index < block_count; block_index++){
    array_add(&has_already, wdup(-1));
    array_add(&work, wdup(-1));
  }

  array worklist; // block_entry_instr*
  ARRAY(worklist);
  for(word var_index = 0; var_index < graph_variable_count(graph); var_index++){
    for(word block_index = 0; block_index < block_count; block_index++){
      if(bit_vector_contains(assigned_vars->data[block_index], var_index)){
        *((word*) work.data[block_index]) = var_index;
        array_add(&worklist, preorder->data[block_index]);
      }
    }

    while(!array_is_empty(&worklist)){
      block_entry_instr* current = array_pop(&worklist);
      for(iterator* it = ((iterator*) bit_vector_iterator_new(dominance->data[current->preorder_num]));
          !it->done(it);
          it->advance(it)){
        word index = ((word) it->current(it));
        if((*((word*) has_already.data[index])) < var_index){
          block_entry_instr* block = preorder->data[index];
          phi_instr* phi = join_entry_instr_insert_phi(container_of(block, join_entry_instr, block), var_index, graph_variable_count(graph));
          phi->is_alive = TRUE;
          array_add(live_phis, phi);
          *((word*) has_already.data[index]) = var_index;
          if((*((word*) work.data[index])) < var_index){
            *((word*) work.data[index]) = var_index;
            array_add(&worklist, block);
          }
        }
      }
    }
  }
}

static void
graph_rename_recursive(flow_graph* graph, block_entry_instr* block, array* /* definition* */ env, array* /* phi_instr* */ live_phis, variable_liveness* analysis){
  if(block->type == kJoinEntryBlock){
    join_entry_instr* join = container_of(block, join_entry_instr, block);
    if(join->phis != NULL){
      for(word i = 0; i < join->phis->size; i++){
        phi_instr* phi = join->phis->data[i];
        if(phi != NULL){
          env->data[i] = phi;
          graph_alloc_ssa_indexes(graph, ((definition*) phi));
        }
      }
    }
  }

  definition* cnull = graph_get_constant(graph, &NIL);

  bit_vector* live_in = liveness_get_live_in(((liveness_analysis*) analysis), block);
  for(word i = 0; i < graph_variable_count(graph); i++){
    if(!bit_vector_contains(live_in, i)){
      env->data[i] = cnull;
    }
  }

  FORWARD_ITER(block){
    if(instr_is_definition(it)){
      definition* defn = container_of(it, definition, instr);
      defn->ssa_temp_index = graph_alloc_ssa_temp_index(graph);
    }

    /*
     * TODO: Renaming Environment?
    for(word i = instr_input_count(it) - 1; i >= 0; i--){
      input* in = instr_input_at(it, i);
      definition* reach = array_pop(env);
      definition* in_defn = in->defn;
      if(in_defn != reach){
        in->defn = reach;
        in_defn = reach;
      }
      input_add(in, &in_defn->input_use_list);
    }

    if(instr_is_definition(it)){
      definition* defn = container_of(it, definition, instr);
      if(it->type == kStoreLocalInstr ||
         it->type == kLoadLocalInstr ||
         it->type == kConstantInstr){

        definition* result = NULL;
        if(it->type == kStoreLocalInstr){
          word index = local_var_bit_index(to_store_local_instr(it)->local, ((int) func_num_non_copied_params(graph->func)));
          result = to_store_local_instr(it)->value->defn;

          if(variable_liveness_is_store_alive(analysis, block, to_store_local_instr(it))){
            env->data[index] = result;
          } else{
            env->data[index] = cnull;
          }
        } else if(it->type == kLoadLocalInstr){
          word index = local_var_bit_index(to_load_local_instr(it)->local, ((int) func_num_non_copied_params(graph->func)));
          result = env->data[index];

          if(((instruction*) result)->type == kPhiInstr){
            phi_instr* phi = container_of(result, phi_instr, defn);
            if(!phi->is_alive){
              phi->is_alive = TRUE;
              array_add(live_phis, phi);
            }
          }

          if(variable_liveness_is_last_load(analysis, block, to_load_local_instr(it))){
            env->data[index] = cnull;
          }
        } else{
          result = container_of(it, definition, instr);
          graph_alloc_ssa_indexes(graph, result);
        }

        if(defn->temp_index >= 0){
          array_add(env, result);
        }
      } else{
        if(defn->temp_index >= 0){
          graph_alloc_ssa_indexes(graph, defn);
          array_add(env, defn);
        }
      }
    }
     */
  }

  for(word i = 0; i < block->dominated.size; i++){
    block_entry_instr* b = block->dominated.data[i];
    array new_env; // definition*
    ARRAY(new_env);
    memcpy(&new_env.data, &env->data[0], sizeof(V) * env->size);

    graph_rename_recursive(graph, b, &new_env, live_phis, analysis);
  }

  //TODO: Process Predecessor Phis
}

static void
graph_rename(flow_graph* graph, array* /* phi_instr* */ live_phis, variable_liveness* analysis){
  graph_entry_instr* gentry = graph->graph_entry;

  //TODO: Environment
  array env; // definition*
  array_init(&env, graph_variable_count(graph));

  //TODO: Params

  //TODO: Insert Default Null

  graph_rename_recursive(graph, ((block_entry_instr*) gentry), &env, live_phis, analysis);
}

void
graph_compute_ssa(flow_graph* graph, word vreg){
  graph->current_ssa_temp_index = vreg;

  array dominance; // bit_vector*
  ARRAY(dominance);
  compute_dominators(graph, &dominance);

  variable_liveness variable_liveness;
  variable_liveness_init(&variable_liveness, graph);
  liveness_analyze(((liveness_analysis*) &variable_liveness));

  array live_phis; // phi_instr*
  insert_phis(graph, &graph->preorder, variable_liveness_compute_assigned(&variable_liveness), &dominance, &live_phis);
  graph_rename(graph, &live_phis, &variable_liveness);
}

#define OWNER vis->owner

input*
evis_bind(effect_visitor* vis, definition* defn){
  graph_builder_dealloc_temps(OWNER, instr_input_count(((instruction*) defn)));
  defn->temp_index = graph_builder_alloc_temp(OWNER);
  if(evis_is_empty(vis)){
    vis->entry = ((instruction*) defn);
  } else{
    instr_link(vis->exit, ((instruction*) defn));
  }
  vis->exit = ((instruction*) defn);
  return input_new(defn);
}

void
evis_do(effect_visitor* vis, definition* defn){
  graph_builder_dealloc_temps(OWNER, instr_input_count(((instruction*) defn)));
  if(evis_is_empty(vis)){
    vis->entry = ((instruction*) defn);
  } else{
    instr_link(vis->exit, ((instruction*) defn));
  }
  vis->exit = ((instruction*) defn);
}

void
evis_add_instruction(effect_visitor* vis, instruction* instr){
  graph_builder_dealloc_temps(OWNER, instr_input_count(instr));
  if(evis_is_empty(vis)){
    vis->entry = vis->exit = instr;
  } else{
    instr_link(vis->exit, instr);
    vis->exit = instr;
  }
}

void
evis_add_return_exit(effect_visitor* vis, input* value){
  instruction* ret = return_instr_new(value);
  evis_add_instruction(vis, ret);
  vis->exit = NULL;
}

void
evis_append(effect_visitor* vis, effect_visitor other){
  if(evis_is_empty(&other)) return;
  if(evis_is_empty(vis)){
    vis->entry = other.entry;
  } else{
    instr_link(vis->exit, other.entry);
  }
  vis->exit = other.exit;
}

static void
evis_return_value(ast_node_visitor* vis, input* value){
  // Fallthrough
}

static void
evis_return_definition(ast_node_visitor* vis, definition* defn){
  if(((instruction*) defn)->type != kConstantInstr){
    evis_do(EVIS(vis), defn);
  }
}

static void
evis_visit_literal(ast_node_visitor* vis, ast_node* node){
  definition* defn = ((definition*) constant_instr_new(node->as.literal.value));
  vis_return_definition(vis, defn);
}

static void
evis_visit_sequence(ast_node_visitor* vis, ast_node* node){
  word i = 0;
  while(evis_is_open(EVIS(vis)) && (i < node->as.sequence.children->size)){
    effect_visitor evis;
    evis_init(&evis, EVIS(vis)->owner);
    ast_visit(((ast_node_visitor*) &evis), node->as.sequence.children->data[i++]);
    evis_append(EVIS(vis), evis);
    if(!evis_is_open(EVIS(vis))){
      break;
    }
  }
}

static void
evis_visit_return(ast_node_visitor* vis, ast_node* node){
  value_visitor for_value;
  vvis_init(&for_value, EVIS(vis)->owner);

  ast_visit(((ast_node_visitor*) &for_value), node->as.ret.value);

  evis_append(EVIS(vis), for_value.visitor);
  evis_add_return_exit(EVIS(vis), for_value.value);
}

static void
evis_visit_store_local(ast_node_visitor* vis, ast_node* node){
  value_visitor for_value;
  vvis_init(&for_value, EVIS(vis)->owner);

  ast_visit(((ast_node_visitor*) &for_value), node->as.store_local.value);

  evis_append(EVIS(vis), for_value.visitor);
  definition* defn = ((definition*) store_local_instr_new(node->as.store_local.local, for_value.value));
  vis_return_definition(vis, defn);
}

static void
evis_visit_load_local(ast_node_visitor* vis, ast_node* node){
  // Fallthrough
}

static void
evis_visit_binary_op(ast_node_visitor* vis, ast_node* node){
  value_visitor for_left_value;
  vvis_init(&for_left_value, EVIS(vis)->owner);
  ast_visit(((ast_node_visitor*) &for_left_value), node->as.binary_op.left);
  evis_append(EVIS(vis), for_left_value.visitor);

  value_visitor for_right_value;
  vvis_init(&for_right_value, EVIS(vis)->owner);
  ast_visit(((ast_node_visitor*) &for_right_value), node->as.binary_op.right);
  evis_append(EVIS(vis), for_right_value.visitor);

  vis_return_definition(vis, ((definition*) binary_op_instr_new(node->as.binary_op.oper, for_left_value.value, for_right_value.value)));
}

#define BIND(func) evis->visitor.visit_##func = evis_visit_##func;

void
evis_init(effect_visitor* evis, flow_graph_builder* owner){
  evis->owner = owner;
  evis->entry = evis->exit = NULL;
  evis->return_value = &evis_return_value;
  evis->return_definition = &evis_return_definition;
  BIND(literal);
  BIND(sequence);
  BIND(return);
  BIND(store_local);
  BIND(load_local);
  BIND(binary_op);
}

static void
vvis_return_value(ast_node_visitor* vis, input* value){
  VVIS(vis)->value = value;
}

static void
vvis_return_definition(ast_node_visitor* vis, definition* defn){
  VVIS(vis)->value = evis_bind(EVIS(vis), defn);
}

static void
vvis_visit_load_local(ast_node_visitor* vis, ast_node* node){
  definition* defn;
  if(node->as.load_local.local->value != NULL){
    defn = ((definition*) constant_instr_new(node->as.load_local.local->value));
  } else{
    defn = ((definition*) load_local_instr_new(node->as.load_local.local));
  }

  vis_return_definition(vis, defn);
}

#undef BIND
#define BIND(func) vis->visitor.visitor.visit_##func = vvis_visit_##func;

void
vvis_init(value_visitor* vis, flow_graph_builder* owner){
  evis_init(((effect_visitor*) vis), owner);
  vis->value = NULL;
  vis->visitor.return_value = &vvis_return_value;
  vis->visitor.return_definition = &vvis_return_definition;
}

void
graph_builder_init(flow_graph_builder* builder, function* func){
  builder->func = func;
  builder->ast = func->ast;
  builder->temp_count = 0x0;
}

static instruction*
append_fragment(block_entry_instr* entry, effect_visitor vis){
  if(evis_is_empty(&vis)) return ((instruction*) entry);
  instr_link(((instruction*) entry), vis.entry);
  return vis.exit;
}

flow_graph*
graph_build(flow_graph_builder* builder){
  instruction* normal = target_entry_instr_new();
  builder->entry = to_graph_entry_instr(graph_entry_instr_new(builder->func, to_target_entry_instr(normal)));

  effect_visitor for_effect;
  evis_init(&for_effect, builder);

  ast_visit(((ast_node_visitor*) &for_effect), builder->ast);
  append_fragment(((block_entry_instr*) normal), for_effect);

  flow_graph* graph = malloc(sizeof(flow_graph));
  graph_init(graph, builder->func, builder->entry);
  return graph;
}