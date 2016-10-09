#ifndef MUN_LSRA_H
#define MUN_LSRA_H

#if !defined(MUN_REGALLOC_H)
#error "Please #include <mun/codegen/regalloc.h> directly"
#endif

#include "../common.h"

HEADER_BEGIN

#include "../location.h"
#include "graph.h"
#include "liveness.h"

typedef struct _live_range live_range;

typedef struct{
  location_kind kind;

  word num_of_regs;
  word spill_count;
  word vreg_count;
  word cpu_spill_slot_count;

  flow_graph* graph;

  array live_ranges; // live_range*
  array unallocated_cpu; // live_range*
  array unallocated_fpu; // live_range*
  array spilled; // live_range*
  array instructions; // instruction*
  array registers; // array[live_range*]*
  array blocked_registers; // bool
  array unallocated; // live_range*
  array postorder; // block_entry_instr*
  array block_order; // block_entry_instr*
  array spill_slots; // word
  array quad_spill_slots; // bool
  array untagged_spill_slots; // bool

  live_range* cpu_regs[kNumberOfCpuRegisters];
  bool blocked_cpu_regs[kNumberOfCpuRegisters];

  live_range* fpu_regs[kNumberOfFpuRegisters];
  bool blocked_fpu_regs[kNumberOfFpuRegisters];

  ssa_liveness liveness;
} flow_graph_allocator;

static const word kDoubleSpillFactor = sizeof(double) / sizeof(word);

void flow_graph_alloc_init(flow_graph_allocator* alloc, flow_graph* graph);
void flow_graph_alloc_regs(flow_graph_allocator* alloc);

HEADER_END

#endif