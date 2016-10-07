#include <mun/codegen/instruction.h>
#include "all.h"

static const instruction_definition* kDefinitions[] = {
    &kGraphEntryDefinition, // GraphEntry
    &kTargetEntryDefinition, // TargetEntry
    NULL, // JoinEntry
    NULL, // Goto
    NULL, // ParallelMove
    &kBoxDefinition, // Box
    &kUnboxDefinition, // Unbox
    NULL, // Phi
    NULL, // Drop
    &kConstantDefinition, // Constant
    &kReturnDefinition, // Return
    &kBinaryOpDefinition, // BinaryOp
    &kStoreLocalDefinition, // StoreLocal
    NULL, // LoadLocal
};

MUN_INLINE void
instr_init(instruction* self, instruction_type type, void* ptr){
  self->lifetime_pos = 0x0;
  self->next = self->prev = NULL;
  self->locations = NULL;
  self->type = type;
  self->ptr = ptr;
}

MUN_INLINE void
block_init(block_entry_instr* block, block_type type){
  block->type = type;
  block->last = NULL;
}

input*
instr_input_at(instruction* instr, word index){
  const instruction_definition* definition = kDefinitions[instr->type];
  return definition != NULL &&
         definition->input_at != NULL ?
         definition->input_at(instr, index) :
         NULL;
}

block_entry_instr*
instr_successor_at(instruction* instr, word index){
  const instruction_definition* definition = kDefinitions[instr->type];
  if(definition != NULL &&
     definition->successor_at != NULL){
    return definition->successor_at(instr, index);
  }
  return NULL;
}

word
instr_successor_count(instruction* instr){
  const instruction_definition* definition = kDefinitions[instr->type];
  if(definition != NULL && definition->successor_count != NULL){
    return definition->successor_count(instr);
  }
  return 0;
}

word
instr_input_count(instruction* instr){
  const instruction_definition* definition = kDefinitions[instr->type];
  if(definition != NULL && definition->input_count != NULL){
    return definition->input_count(instr);
  }
  return 0;
}

void
instr_set_input_at(instruction* instr, word index, input* value){
  value->instr = instr;
  value->index = index;
  const instruction_definition* definition = kDefinitions[instr->type];
  if(definition != NULL && definition->set_input_at != NULL) definition->set_input_at(instr, index, value);
}

void
instr_compile(instruction* instr, asm_buff* code){
  const instruction_definition* definition = kDefinitions[instr->type];
  if(instr->locations == NULL) instr->locations = (definition != NULL && definition->make_location_summary != NULL) ?
                                                  definition->make_location_summary(instr) :
                                                  NULL;
  if(definition != NULL &&
     definition->compile != NULL){
    definition->compile(instr, code);
  }
}

char*
instr_name(instruction* instr){
  const instruction_definition* definition = kDefinitions[instr->type];
  if(definition != NULL && definition->name != NULL){
    return definition->name();
  } else{
    char* data = malloc(5);
    snprintf(data, 4, "%d", instr->type);
    data[4] = '\0';
    return data;
  }
}

static const block_definition* kBlockDefinitions[] = {
    &kGraphEntryBlockDefinition,
};

void
block_clear_predecessors(block_entry_instr* block){
  const block_definition* definition = kBlockDefinitions[block->type];
  if(definition != NULL && definition->clear_predecessors != NULL){
    definition->clear_predecessors(block);
  }
}

void
block_add_predecessor(block_entry_instr* block, block_entry_instr* predecessor){
  const block_definition* definition = kBlockDefinitions[block->type];
  if(definition != NULL && definition->add_predecessor != NULL){
    definition->add_predecessor(block, predecessor);
  }
}

MUN_INLINE bool
is_marked(block_entry_instr* block, array* preorder){
  word index = block->preorder_num;
  return index >= 0 &&
         index < preorder->size &&
         preorder->data[index] == block;
}

bool
block_discover_blocks(block_entry_instr* block, block_entry_instr* predecessor, array* /* block_entry_instr* */ preorder, array* /* word */ parent){
  if(is_marked(block, preorder)) {
    block_add_predecessor(block, predecessor);
    return FALSE;
  }

  block_clear_predecessors(block);
  if(predecessor != NULL) block_add_predecessor(block, predecessor);

  word parent_num = (predecessor == NULL) ?
                    -1 :
                    predecessor->preorder_num;

  array_add(parent, wdup(parent_num));
  block->preorder_num = preorder->size;
  array_add(preorder, block);

  instruction* last = ((instruction*) block);
  FORWARD_ITER(block){
    last = it;
  }
  block->last = last;
  return TRUE;
}

instruction*
constant_instr_new(instance* value){
  constant_instr* c = malloc(sizeof(constant_instr));
  instr_init(((instruction*) c), kConstantInstr, c);
  c->value = value;
  return ((instruction*) c);
}

instruction*
return_instr_new(input* value){
  return_instr* ret = malloc(sizeof(return_instr));
  instr_init(((instruction*) ret), kReturnInstr, ret);
  instr_set_input_at(((instruction*) ret), 0, value);
  return ((instruction*) ret);
}

instruction*
graph_entry_instr_new(function* func, target_entry_instr* normal){
  graph_entry_instr* gentry = malloc(sizeof(graph_entry_instr));
  instr_init(((instruction*) gentry), kGraphEntryInstr, gentry);
  block_init(((block_entry_instr*) gentry), kGraphEntryBlock);
  gentry->func = func;
  gentry->normal_entry = normal;
  gentry->entry_count = 0x0;
  gentry->spill_slot_count = 0x0;
  gentry->fixed_slot_count = 0x0;
  ARRAY(gentry->definitions);
  return ((instruction*) gentry);
}

instruction*
target_entry_instr_new(){
  target_entry_instr* target = malloc(sizeof(target_entry_instr));
  instr_init(((instruction*) target), kTargetEntryInstr, target);
  block_init(((block_entry_instr*) target), kTargetEntryBlock);
  target->predecessor = NULL;
  return ((instruction*) target);
}

instruction*
store_local_instr_new(local_variable* local, input* value){
  store_local_instr* store = malloc(sizeof(store_local_instr));
  instr_init(((instruction*) store), kStoreLocalInstr, store);
  store->is_dead = store->is_last = FALSE;
  store->local = local;
  instr_set_input_at(((instruction*) store), 0, value);
  return ((instruction*) store);
}

instruction*
load_local_instr_new(local_variable* local){
  load_local_instr* load = malloc(sizeof(load_local_instr));
  instr_init(((instruction*) load), kLoadLocalInstr, load);
  load->is_last = FALSE;
  load->local = local;
  return ((instruction*) load);
}