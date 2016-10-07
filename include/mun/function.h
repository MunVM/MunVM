#ifndef MUN_FUNCTION_H
#define MUN_FUNCTION_H

#include "common.h"

HEADER_BEGIN

#include "ast.h"

typedef struct _function{
  char* name;
  int mods;
  ast_node* ast;
  uword code;
  struct{
    word num_params;
    word num_stack_locals;
    word num_copied_params;
    word first_param_index;
    word first_stack_local_index;
  };
} function;

HEADER_END

#endif