#include <mun/alloc.h>
#include <mun/parser.h>
#include <mun/location.h>
#include <mun/type.h>
#include <mun/ast.h>

static void
nprint_call(native_arguments* args){
  printf("Printing\n");
  printf("%d\n", native_arguments_at(args, 0)->type);
}

int
main(int argc, char** argv){
  mun_alloc* gc = malloc(sizeof(mun_alloc));
  mun_gc_init(gc);

  instance* nprint = function_new(gc, "print", kMOD_NATIVE);
  nprint->as.func->native = &nprint_call;
  nprint->as.func->num_params = 0x1;
  nprint->as.func->ast = sequence_node_new(gc);
  nprint->as.func->ast->as.sequence.scope = local_scope_new(NULL);
  local_scope_add(nprint->as.func->ast->as.sequence.scope, local_var_new("arg"));

  instruction* ncall = native_call_instr_new(nprint->as.func);
  ncall->locations = loc_summary_new(0, 0, CALL);
  loc_init_r(&ncall->locations->output, RAX);

  func_alloc_variables(nprint->as.func);

  asm_buff code;
  asm_init(&code);

  asm_enter_frame(&code, 0x0);
  asm_subq_ri(&code, RSP, kWordSize);

  asm_address slot_addr;
  asm_addr_init_r(&slot_addr, RBP, 0);

  asm_movq_ai(&code, &slot_addr, ((uword) number_new(gc, 3.141592654)));
  instr_compile(ncall, &code);

  asm_leave_frame(&code);
  asm_ret(&code);

  typedef void (*native_func)(void);

  ((native_func) asm_compile(&code))();
  return 0;
}