#ifndef MUN_MOVE_RESOLVER_H
#define MUN_MOVE_RESOLVER_H

#include "../common.h"

HEADER_BEGIN

#include "../array.h"
#include "instruction.h"

typedef struct{
  struct _flow_graph_compiler* compiler;
  array moves; // move_operand*
} move_resolver;

void move_resolver_init(move_resolver* resolver, struct _flow_graph_compiler* compiler);
void move_resolver_compile(move_resolver* resolver, parallel_move_instr* move);
void move_resolver_emit_move(move_resolver* resolver, word index);
void move_resolver_emit_swap(move_resolver* resolver, word index);

HEADER_END

#endif