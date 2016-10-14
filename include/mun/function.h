#ifndef MUN_FUNCTION_H
#define MUN_FUNCTION_H

#include "common.h"

HEADER_BEGIN

#include "native.h"
#include "ast.h"

typedef struct _function{
  char* name;
  int mods;
  ast_node* ast;
  uword code;
  void (*native)(native_arguments* args);
  struct{
    word num_params;
    word num_stack_locals;
    word num_copied_params;
    word first_param_index;
    word first_stack_local_index;
  };
} function;

#define IS_NATIVE(func) ((func->mods & kMOD_NATIVE) == kMOD_NATIVE)

MUN_INLINE void
func_alloc_variables(function* func){
  local_scope* scope = func->ast->as.sequence.scope;
  func->first_param_index = kParamEndSlotFromFp + func->num_params;
  func->first_stack_local_index = kFirstLocalSlotFromFp;
  func->num_copied_params = 0x0;
  int next_free_frame_index = local_scope_alloc_variables(scope, ((int) func->first_param_index), ((int) func->num_params), ((int) func->first_stack_local_index));
  func->num_stack_locals = func->first_stack_local_index - next_free_frame_index;
}

MUN_INLINE word
func_num_non_copied_params(function* func){
  return (func->num_copied_params == 0) ?
         func->num_params :
         0;
}

HEADER_END

#endif