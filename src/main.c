#include <mun/alloc.h>
#include <mun/parser.h>
#include <mun/location.h>
#include <mun/codegen/flow_graph.h>
#include <mun/codegen/interference_graph.h>
#include "codegen/all.h"

mun_alloc* GC;

int
main(int argc, char** argv){
  GC = malloc(sizeof(mun_alloc));
  mun_gc_init(GC);

  instance* script = script_new(GC, argv[1]);

  parser* p = parser_new(GC);
  parse(p, script);

  flow_graph_builder builder;
  graph_builder_init(&builder, script->as.script.main);

  flow_graph* graph = graph_build(&builder);
  graph_discover_blocks(graph);
  graph_compute_ssa(graph, 0x0);

  interference_graph rig;
  interference_graph_init(&rig, graph);
  interference_graph_compute(&rig);

  asm_buff code;
  asm_init(&code);

  FORWARD_ITER(graph->graph_entry->normal_entry){
    instr_compile(it, &code);
  }

  typedef instance* (*native_function)(void);

  instance* result = ((native_function) asm_compile(&code))();

  printf("%f\n", result->as.number);
  return 0;
}