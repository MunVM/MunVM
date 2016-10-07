#ifndef MUN_TYPE_H
#define MUN_TYPE_H

#include "common.h"
#include "sstream.h"

HEADER_BEGIN

typedef enum{
  kMOD_NONE = 1 << 0,
  kMOD_NATIVE = 1 << 1,
} instance_modifier;

typedef enum{
  kIllegalType = -1,

  kStringType = 1 << 0,
  kNumberType = 1 << 1,
  kFunctionType = 1 << 2,
  kBooleanType = 1 << 3,
  kNilType = 1 << 4,
  kTableType = 1 << 5,
  kScriptType = 1 << 6
} instance_type;

typedef struct _instance{
  instance_type type;
  union{
    bool boolean;
    double number;
    sstream* string;
    struct _table* table;
    struct _function* func;
    struct{
      char* url;
      struct _function* main;
    } script;
  } as;
  uint32_t (*hashcode)(struct _instance*);
} instance;

extern instance NIL;

instance* string_new(mun_alloc* alloc, char* data);
instance* table_new(mun_alloc* alloc, word len);
instance* bool_new(mun_alloc* alloc, bool value);
instance* number_new(mun_alloc* alloc, double value);
instance* function_new(mun_alloc* alloc, char* name, int mods);
instance* script_new(mun_alloc* alloc, char* url);

HEADER_END

#endif