#ifndef MUN_COMPILER_H
#define MUN_COMPILER_H

#include "../common.h"

HEADER_BEGIN

#include "../asm.h"
#include "flow_graph.h"
#include "move_resolver.h"

typedef struct _flow_graph_compiler{
  asm_buff code;
  flow_graph* graph;
  function* func;
  move_resolver resolver;
  array blocks; // block_entry_instr*
  block_entry_instr* block;
} flow_graph_compiler;

MUN_INLINE void
flow_graph_compiler_init(flow_graph_compiler* compiler, flow_graph* graph, function* func){
  compiler->func = func;
  compiler->graph = graph;
  compiler->blocks = graph->reverse_postorder;
  move_resolver_init(&compiler->resolver, compiler);
  asm_init(&compiler->code);
}

void flow_graph_compiler_emit_epilogue(flow_graph_compiler* compiler, instruction* instr);
void flow_graph_compiler_emit_prologue(flow_graph_compiler* compiler, instruction* instr);
void flow_graph_compiler_emit_entry(flow_graph_compiler* compiler);
void flow_graph_compiler_compile(flow_graph_compiler* compiler);
void flow_graph_compiler_visit_blocks(flow_graph_compiler* compiler);

HEADER_END

#endif