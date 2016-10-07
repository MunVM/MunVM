#ifndef MUN_TABLE_H
#define MUN_TABLE_H

#include "common.h"
#include "type.h"

HEADER_BEGIN

typedef uint32_t table_key;
typedef struct _table table;

table* table_create(word size);

bool table_put(table* tbl, table_key key, instance* item);
bool table_remove(table* tbl, table_key key);

instance* table_get(table* tbl, table_key key);

table_key string_key(char* data);

#define KEY(inst) inst->hashcode(inst)

HEADER_END

#endif