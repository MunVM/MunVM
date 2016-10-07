#include <mun/codegen/graph.h>
#include <mun/ast.h>
#include <mun/scope.h>

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