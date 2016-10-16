#ifndef MUN_INTERFERENCE_H
#define MUN_INTERFERENCE_H

#include "../common.h"

HEADER_BEGIN

#include "flow_graph.h"

typedef struct{
  word start;
  word end;
  word vreg;
} live_range;

typedef struct{
  array live_ranges; // live_range*
  array active; // live_range*
  array register_pool; // word
  array register_maps; // word
  array instructions; // instruction*
  array* postorder; // block_entry_instr*
  flow_graph* graph;
} interference_graph;

void interference_graph_init(interference_graph* rig, flow_graph* cfg);
void interference_graph_compute(interference_graph* rig);

HEADER_END

#endif