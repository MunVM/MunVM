#include <mun/alloc.h>
#include <mun/parser.h>
#include <mun/codegen/graph.h>

int
main(int argc, char** argv){
  mun_alloc* gc = malloc(sizeof(mun_alloc));
  mun_gc_init(gc);

  instance* load = script_new(gc, argv[1]);

  parser* p = parser_new(gc);
  parse(p, load);

  flow_graph_builder builder;
  graph_builder_init(&builder, load->as.script.main);

  flow_graph* graph = graph_build(&builder);
  graph_discover_blocks(graph);

  int count = 0x0;
  FORWARD_ITER(graph_entry_instr_normalize(((instruction*) graph->graph_entry))){
    printf("%d: %s\n", ((count++) + 1), instr_name(it));
  }
  return 0;
}