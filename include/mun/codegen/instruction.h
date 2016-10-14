#ifndef MUN_CODEGEN_COMMON_H
#define MUN_CODEGEN_COMMON_H

#include "../common.h"

HEADER_BEGIN

#include "../type.h"
#include "../array.h"
#include "../function.h"
#include "../asm.h"

#define FOR_EACH_INSTRUCTION(V) \
  V(GraphEntry) \
  V(TargetEntry) \
  V(JoinEntry) \
  V(Goto) \
  V(ParallelMove) \
  V(Box) \
  V(Unbox) \
  V(Phi) \
  V(Drop) \
  V(Constant) \
  V(Return) \
  V(BinaryOp) \
  V(StoreLocal) \
  V(LoadLocal) \
  V(NativeCall)

typedef enum{
#define DEFINE_TYPE(Name) k##Name##Instr,
  FOR_EACH_INSTRUCTION(DEFINE_TYPE)
#undef DEFINE_TYPE
} instruction_type;

typedef enum{
  kUnboxed,
  kBoxed,
  kTagged,
  kNone
} representation;

typedef struct _input input;

typedef struct _instruction{
  struct _instruction* next;
  struct _instruction* prev;
  void* ptr;
  word lifetime_pos;
  struct _location_summary* locations;
  instruction_type type;
} instruction;

bool instr_is_definition(instruction* instr);

input* instr_input_at(instruction* instr, word index);

struct _block_entry_instr* instr_successor_at(instruction* instr, word index);

word instr_successor_count(instruction* instr);
word instr_input_count(instruction* instr);

void instr_initialize_location_summary(instruction* instr);
void instr_set_input_at(instruction* instr, word index, input* value);
void instr_compile(instruction* instr, asm_buff* code);

char* instr_name(instruction* instr);

MUN_INLINE void
instr_link(instruction* instr, instruction* next){
  instr->next = next;
  next->prev = instr;
}

typedef struct _definition{
  instruction instr;

  word temp_index;
  word ssa_temp_index;
  input* input_use_list;
} definition;

typedef enum{
  kGraphEntryBlock,
  kTargetEntryBlock,
  kJoinEntryBlock,
} block_type;

typedef struct _block_entry_instr{
  instruction instr;

  word preorder_num;
  word postorder_num;
  word start_pos;
  word end_pos;
  word offset;

  instruction* last;

  array dominated; // block_entry_instr*

  struct _block_entry_instr* dominator;

  block_type type;
} block_entry_instr;

void block_clear_predecessors(block_entry_instr* block);
void block_add_predecessor(block_entry_instr* block, block_entry_instr* predecessor);

block_entry_instr* block_predecessor_at(block_entry_instr* block, word index);

word block_predecessor_count(block_entry_instr* block);

bool block_discover_blocks(block_entry_instr* block, block_entry_instr* predecessor, array* /* block_entry_instr* */ preorder, array* /* word */ parent);

MUN_INLINE void
block_add_dominated(block_entry_instr* block, block_entry_instr* dominated){
  dominated->dominator = block;
  array_add(&block->dominated, dominated);
}

#define FORWARD_ITER(Block) \
  for(instruction* it = ((instruction*) Block)->next; \
      it != NULL; \
      it = it->next)

#define BACKWARD_ITER(Block) \
  for(instruction* it = Block->last; \
      it != ((instruction*) Block); \
      it = it->prev)

struct _input{
  struct _input* next;
  struct _input* prev;

  definition* defn;
  instruction* instr;

  word index;
};

MUN_INLINE input*
input_new(definition* defn){
  input* in = malloc(sizeof(input));
  in->instr = NULL;
  in->next = in->prev = NULL;
  in->defn = defn;
  return in;
}

MUN_INLINE void
input_remove_from_list(input* in){
  definition* defn = in->defn;
  input* next = in->next;
  if(in == defn->input_use_list){
    defn->input_use_list = next;
    if(next != NULL) next->prev = NULL;
  } else{
    input* prev = in->prev;
    prev->next = next;
    if(next != NULL) next->prev = prev;
  }

  in->next = NULL;
  in->prev = NULL;
}

MUN_INLINE void
input_add(input* in, input** list){
  input* next = *list;
  *list = in;
  in->next = next;
  in->prev = NULL;
  if(next != NULL) next->prev = in;
}

MUN_INLINE void
input_bind(input* in, definition* defn){
  input_remove_from_list(in);
  in->defn = defn;
  input_add(in, &defn->input_use_list);
}

#define DECLARE_INSTRUCTION(Name, ...) \
  typedef struct _##Name##_instr Name##_instr; \
  instruction* Name##_instr_new(__VA_ARGS__); \
  MUN_INLINE Name##_instr* to_##Name##_instr(instruction* instr){ \
    return ((Name##_instr*) instr->ptr); \
  }
DECLARE_INSTRUCTION(return, input*);
DECLARE_INSTRUCTION(constant, instance*);
DECLARE_INSTRUCTION(binary_op, int, input*, input*);
DECLARE_INSTRUCTION(box, representation, input*);
DECLARE_INSTRUCTION(unbox, representation, input*);
DECLARE_INSTRUCTION(target_entry);
DECLARE_INSTRUCTION(graph_entry, function*, target_entry_instr*);
DECLARE_INSTRUCTION(join_entry);
DECLARE_INSTRUCTION(parallel_move);
DECLARE_INSTRUCTION(goto, join_entry_instr*);
DECLARE_INSTRUCTION(store_local, local_variable*, input*);
DECLARE_INSTRUCTION(load_local, local_variable*);
DECLARE_INSTRUCTION(phi, join_entry_instr*, word);
DECLARE_INSTRUCTION(native_call, function*);
#undef DECLARE_INSTRUCTION

instance* constant_instr_get_value(instruction* instr);

instruction* graph_entry_instr_normalize(instruction* instr);

phi_instr* join_entry_instr_insert_phi(join_entry_instr* join, word var_index, word var_count);

bool parallel_move_instr_is_redundant(parallel_move_instr* moves);

HEADER_END

#endif