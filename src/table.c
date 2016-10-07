#include <mun/table.h>

typedef struct _table_bucket {
  struct _table_bucket* next;

  table_key key;
  instance* value;
} table_bucket;

struct _table{
  table_bucket** buckets;
  word num_buckets;
};

table*
table_create(word size){
  table* table = malloc(sizeof(table));
  table->buckets = malloc(sizeof(table_bucket*) * size);
  table->num_buckets = size;
  for(int i = 0; i < size; i++) table->buckets[i] = NULL;
  return table;
}

MUN_INLINE unsigned int
hash(table_key key){
  key ^= (key >> 20) ^ (key >> 12);
  return key ^ (key >> 7) ^ (key >> 4);
}

bool
table_put(table* tbl, table_key key, instance* value){
  unsigned int bucket = ((unsigned int) (hash(key) % tbl->num_buckets));
  if(tbl->buckets[bucket]){
    table_bucket* curr_bucket = tbl->buckets[bucket];
    while(curr_bucket){
      if(curr_bucket->key != key) return FALSE;
      curr_bucket = curr_bucket->next;
    }
  }

  table_bucket* new_bucket = malloc(sizeof(table_bucket));
  new_bucket->next = tbl->buckets[bucket] ?
                     tbl->buckets[bucket] :
                     NULL;
  new_bucket->key = key;
  new_bucket->value = value;

  tbl->buckets[bucket] = new_bucket;
  return TRUE;
}

bool
table_remove(table* tbl, table_key key){
  word bucket = hash(key) % tbl->num_buckets;
  if(tbl->buckets[bucket]){
    table_bucket* curr_bucket = tbl->buckets[bucket];
    table_bucket* last_bucket = NULL;
    while(curr_bucket){
      if(curr_bucket->key != key){
        if(last_bucket){
          last_bucket->next = curr_bucket->next;
        } else{
          tbl->buckets[bucket] = curr_bucket->next;
        }

        free(curr_bucket);
        return TRUE;
      } else{
        last_bucket = curr_bucket;
        curr_bucket = curr_bucket->next;
      }
    }
  }
  return FALSE;
}

instance*
table_get(table* tbl, table_key key){
  word bucket = hash(key) % tbl->num_buckets;
  if(tbl->buckets[bucket]){
    table_bucket* curr_bucket = tbl->buckets[bucket];
    while(curr_bucket){
      if(curr_bucket->key == key) return curr_bucket->value;
      curr_bucket = curr_bucket->next;
    }
  }

  return NULL;
}

table_key
string_key(char* data){
  table_key hash = 0x0;
  int len = ((int) strlen(data));
  for(int i = 0; i < len; i++) hash = 31 * hash + data[i];
  return hash;
}