#ifndef MUN_ALL_H
#define MUN_ALL_H

#include <mun/codegen/instruction.h>
#include <mun/location.h>

typedef struct _instruction_definition{
  void (*set_input_at)(instruction*, word, input*);
  void (*compile)(instruction*, asm_buff*);

  representation (*get_representation)(instruction*);
  representation (*get_input_representation)(instruction*, word);

  location_summary* (*make_location_summary)(instruction*);

  definition* (*argument_at)(instruction*, word);

  block_entry_instr* (*successor_at)(instruction*, word);

  word (*argument_count)(instruction*);
  word (*input_count)(instruction*);
  word (*successor_count)(instruction*);

  input* (*input_at)(instruction*, word);

  char* (*name)();
} instruction_definition;

typedef struct _block_definition{
  word (*predecessor_count)(block_entry_instr*);
  void (*add_predecessor)(block_entry_instr*, block_entry_instr*);
  void (*clear_predecessors)(block_entry_instr*);
  block_entry_instr* (*predecessor_at)(block_entry_instr*, word);
} block_definition;

#define DECLARE_DEFINITION(Name) \
  extern const struct _instruction_definition k##Name##Definition;
DECLARE_DEFINITION(Constant);
DECLARE_DEFINITION(Return);
DECLARE_DEFINITION(BinaryOp);
DECLARE_DEFINITION(Box);
DECLARE_DEFINITION(Unbox);
DECLARE_DEFINITION(GraphEntry);
DECLARE_DEFINITION(TargetEntry);
DECLARE_DEFINITION(JoinEntry);
DECLARE_DEFINITION(StoreLocal);
DECLARE_DEFINITION(LoadLocal);
DECLARE_DEFINITION(Phi);
DECLARE_DEFINITION(ParallelMove);
DECLARE_DEFINITION(NativeCall);
#undef DECLARE_DEFINITION

#define DEFINE_BLOCK(Name) \
  const block_definition k##Name##BlockDefinition =

#define DEFINE(Name) \
  const instruction_definition k##Name##Definition =

#define PREDECESSOR_COUNT(Name, Amount) \
  static word \
  Name##_predecessor_count(block_entry_instr* block){ \
    return Amount; \
  }

#define ADD_PREDECESSOR(Name) \
  static void \
  Name##_add_predecessor(block_entry_instr* block, block_entry_instr* pred)

#define PREDECESSOR_AT(Name, Value) \
  static block_entry_instr* \
  Name##_predecessor_at(block_entry_instr* block, word index){ \
    return to_##Name##_instr(((instruction*) block))->Value; \
  }

#define CLEAR_PREDECESSORS(Name) \
  static void \
  Name##_clear_predecessors(block_entry_instr* block)

#define INPUT_COUNT(Name, Amount) \
  static word \
  Name##_input_count(instruction* instr){ \
    return Amount; \
  }

#define INPUT_AT(Name, Value) \
  static input* \
  Name##_input_at(instruction* instr, word index){ \
    return to_##Name##_instr(instr)->Value; \
  } \
  static void \
  Name##_set_input_at(instruction* instr, word index, input* value){ \
    to_##Name##_instr(instr)->Value = value; \
  }

#define NAME(Name) \
  static char* \
  Name##_name(){ \
    return #Name; \
  }

#define SUCCESSOR_COUNT(Name, Amount) \
  static word \
  Name##_successor_count(instruction* instr){ \
    return Amount; \
  }

#define SUCCESSOR_AT(Name) \
  static block_entry_instr* \
  Name##_successor_at(instruction* instr, word index)

#if defined(ARCH_IS_X64)
#include "x64/compile.h"
#else
#error "Unknown CPU architecture"
#endif

#include "definitions.h"
#include "blocks.h"
#include "parallel_move.h"

#endif