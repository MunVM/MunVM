#include <mun/codegen/compiler.h>

void
flow_graph_compiler_visit_blocks(flow_graph_compiler* compiler){
  //TODO: CompactBlocks

  for(word i = 0; i < compiler->blocks.size; i++){
    block_entry_instr* block = compiler->block = compiler->blocks.data[i];
    instr_compile(((instruction*) block), &compiler->code);

    FORWARD_ITER(block){
      if(it->type == kParallelMoveInstr){
        move_resolver_compile(&compiler->resolver, to_parallel_move_instr(it));
      } else{
        flow_graph_compiler_emit_prologue(compiler, it);
        instr_compile(it, &compiler->code);
        flow_graph_compiler_emit_epilogue(compiler, it);
      }
    }

    compiler->block = NULL;
  }
}