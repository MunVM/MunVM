#ifndef MUN_BLOCKS_H
#define MUN_BLOCKS_H

#if !defined(MUN_ALL_H)
#error "Please #include <mun/all.h> directly"
#endif

struct _graph_entry_instr{
  block_entry_instr block;

  target_entry_instr* normal_entry;

  function* func;

  word entry_count;
  word spill_slot_count;
  word fixed_slot_count;

  array definitions; // definition*
};

struct _target_entry_instr{
  block_entry_instr block;

  block_entry_instr* predecessor;
};

struct _join_entry_instr{
  block_entry_instr block;

  array predecessors; // block_entry_instr*
  array* phis; // phi_instr*
};

#define DECLARE_BLOCK(Name) \
  extern const struct _block_definition k##Name##BlockDefinition;
DECLARE_BLOCK(GraphEntry);
DECLARE_BLOCK(TargetEntry);
DECLARE_BLOCK(JoinEntry);
#undef DECLARE_BLOCK

#endif