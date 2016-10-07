#ifndef MUN_SCOPE_H
#define MUN_SCOPE_H

#include "common.h"

HEADER_BEGIN

#include "array.h"
#include "type.h"
#include "stack_frame.h"

typedef struct{
  char* name;
  struct _local_scope* owner;
  int index;
  instance* value;
} local_variable;

local_variable* local_var_new(char* name);

MUN_INLINE bool
local_var_is_constant(local_variable* local){
  return local->value != NULL;
}

MUN_INLINE bool
local_var_has_index(local_variable* local){
  return local->index != -1;
}

MUN_INLINE int
local_var_bit_index(local_variable* local, int param_count){
  return local->index > 0 ?
         param_count - (local->index - kParamEndSlotFromFp) :
         param_count - (local->index - kFirstLocalSlotFromFp);
}

typedef struct _local_scope{
  struct _local_scope* parent;
  struct _local_scope* child;
  struct _local_scope* sibling;

  array variables; // local_variable*
} local_scope;

local_scope* local_scope_new(local_scope* parent);

bool local_scope_add(local_scope* scope, local_variable* var);

local_variable* local_scope_local_lookup(local_scope* scope, char* name);
local_variable* local_scope_lookup(local_scope* scope, char* name);

MUN_INLINE bool
local_scope_insert_param(local_scope* scope, word index, local_variable* var){
  if(local_scope_local_lookup(scope, var->name) != NULL) return FALSE;
  array_insert(&scope->variables, index, var);
  var->owner = scope;
  return TRUE;
}

int local_scope_alloc_variables(local_scope* scope, int first_param_index, int num_params, int first_frame_index);

HEADER_END

#endif