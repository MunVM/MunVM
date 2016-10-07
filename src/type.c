#include <mun/type.h>
#include <mun/alloc.h>
#include <mun/table.h>
#include <mun/ast.h>
#include <mun/function.h>

MUN_INLINE instance*
instance_new(mun_alloc* alloc, instance_type type, uint32_t (*hash_func)(instance*)){
  instance* inst = mun_gc_alloc(alloc, sizeof(instance));
  inst->type = type;
  inst->hashcode = hash_func;
  return inst;
}

static table_key
string_hashcode(instance* val){
  uint32_t hash = 0x0;
  int len = ((int) val->as.string->size);
  for(int i = 0; i < len; i++) hash = 31 * hash + ((char) val->as.string->data[i]);
  return hash;
}

instance*
string_new(mun_alloc* alloc, char* data){
  instance* str = instance_new(alloc, kStringType, &string_hashcode);
  str->as.string = sstream_new();
  sstream_putstr(str->as.string, data);
  return str;
}

instance*
table_new(mun_alloc* alloc, word len){
  instance* tbl = instance_new(alloc, kTableType, NULL);
  tbl->as.table = table_create(len);
  return tbl;
}

static table_key
boolean_hashcode(instance* value){
  return value->as.boolean ? 1231 : 1237;
}

instance*
bool_new(mun_alloc* alloc, bool value){
  instance* b = instance_new(alloc, kBooleanType, &boolean_hashcode);
  b->as.boolean = value;
  return b;
}

static table_key
number_hashcode(instance* value){
  return ((table_key) value->as.number);
}

instance*
number_new(mun_alloc* alloc, double value){
  instance* n = instance_new(alloc, kNumberType, &number_hashcode);
  n->as.number = value;
  return n;
}

static table_key
function_hashcode(instance* value){
  uint32_t hash = 0x0;
  int len = ((int) strlen(value->as.func->name));
  for(int i = 0; i < len; i++) hash = 31 * hash + value->as.func->name[i];
  return hash;
}

instance*
function_new(mun_alloc* alloc, char* name, int mods){
  instance* func = instance_new(alloc, kFunctionType, &function_hashcode);
  function* f = func->as.func = malloc(sizeof(function));
  f->mods = mods;
  f->name = strdup(name);
  f->code = 0x0;
  f->num_params = 0x0;
  f->num_stack_locals = 0x0;
  f->num_copied_params = 0x0;
  f->first_param_index = 0x0;
  f->first_stack_local_index = 0x0;
  return func;
}

static table_key
script_hashcode(instance* value){
  uint32_t hash = 0x0;
  char* code = value->as.script.url;
  int len = ((int) strlen(code));
  for(int i = 0; i < len; i++) hash = 31 * hash + code[i];
  return hash;
}

instance*
script_new(mun_alloc* alloc, char* url){
  instance* script = instance_new(alloc, kScriptType, &script_hashcode);
  script->as.script.url = strdup(url);
  script->as.script.main = function_new(alloc, "__main__", kMOD_NONE)->as.func;
  script->as.script.main->ast = sequence_node_new(alloc);
  script->as.script.main->ast->as.sequence.scope = local_scope_new(NULL);
  return script;
}

static table_key
nil_hashcode(instance* self){
  return 0x0;
}

instance NIL = {
    kNilType,
    { FALSE },
    &nil_hashcode,
};