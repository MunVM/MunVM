#include <mun/scope.h>
#include <mun/array.h>

bool
local_scope_add(local_scope* scope, local_variable* local){
  if(local_scope_local_lookup(scope, local->name) != NULL) return FALSE;
  array_add(&scope->variables, local);
  if(local->owner == NULL) local->owner = scope;
  return TRUE;
}

local_variable*
local_scope_local_lookup(local_scope* scope, char* name){
  for(int i = 0; i < scope->variables.size; i++){
    local_variable* local = scope->variables.data[i];
    if(strncmp(local->name, name, strlen(local->name)) == 0) return local;
  }
  return NULL;
}

local_variable*
local_scope_lookup(local_scope* scope, char* name){
  local_scope* current_scope = scope;
  while(current_scope != NULL){
    local_variable* local = local_scope_local_lookup(current_scope, name);
    if(local != NULL) return local;
    current_scope = current_scope->parent;
  }
  return NULL;
}

local_scope*
local_scope_new(local_scope* parent){
  local_scope* new_scope = malloc(sizeof(local_scope));
  new_scope->parent = parent;
  new_scope->child = NULL;
  new_scope->sibling = NULL;
  ARRAY(new_scope->variables);
  if(parent != NULL){
    new_scope->sibling = parent->child;
    parent->child = new_scope;
  }
  return new_scope;
}

int
local_scope_alloc_variables(local_scope* scope, int first_param_index, int num_params, int first_frame_index){
  int pos = 0x0;
  int frame_index = first_param_index;

  while(pos < num_params){
    local_variable* local = scope->variables.data[pos];
    pos++;
    local->index = frame_index--;
  }

  frame_index = first_frame_index;
  while(pos < scope->variables.size){
    local_variable* local = scope->variables.data[pos];
    pos++;
    if(local->owner == scope) local->index = frame_index--;
  }

  int min_frame_index = frame_index;

  local_scope* child = scope->child;
  while(child != NULL){
    int child_frame_index = local_scope_alloc_variables(child, 0, 0, frame_index);
    if(child_frame_index < min_frame_index) min_frame_index = child_frame_index;
    child = child->sibling;
  }

  return min_frame_index;
}

local_variable*
local_var_new(char* name){
  local_variable* local = malloc(sizeof(local_variable));
  local->name = strdup(name);
  local->index = -1;
  local->owner = NULL;
  local->value = NULL;
  return local;
}