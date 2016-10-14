#ifndef MUN_DEFINITIONS_H
#define MUN_DEFINITIONS_H

#if !defined(MUN_ALL_H)
#error "Please #include <mun/all.h> directly"
#endif

#include <mun/bitvec.h>

struct _constant_instr{
  definition defn;

  instance* value;
};

struct _return_instr{
  definition defn;

  input* value;
};

struct _binary_op_instr{
  definition defn;

  int operation;
  input* inputs[2];
};

struct _box_instr{
  definition defn;

  representation from;
  input* value;
};

struct _unbox_instr{
  definition defn;

  representation to;
  input* value;
};

struct _store_local_instr{
  definition defn;

  local_variable* local;
  bool is_dead : 1;
  bool is_last : 1;
  input* value;
};

struct _load_local_instr{
  definition defn;

  local_variable* local;
  bool is_last : 1;
};

struct _phi_instr{
  definition defn;

  join_entry_instr* block;
  array inputs; // input*
  bit_vector* reaching;
  bool is_alive : 1;
};

struct _native_call_instr{
  definition defn;

  function* func;
};

#endif