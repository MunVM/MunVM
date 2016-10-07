#ifndef MUN_ALLOC_H
#define MUN_ALLOC_H

#include "common.h"

HEADER_BEGIN

typedef unsigned char byte;

#define GC_MAX_REFS 65536
#define GC_MINCHUNK_SIZE 256
#define GC_MINHEAP_SIZE (1024 * 64)
#define GC_MAJHEAP_SIZE (1024 * 1024 * 32)
#define GC_MINCHUNKS (GC_MINHEAP_SIZE / GC_MINCHUNK_SIZE)

struct _mun_alloc{
  byte minor_heap[GC_MINHEAP_SIZE];
  byte major_heap[GC_MAJHEAP_SIZE];

  int free_chunk;
  int refs_count;

  void* backpatch[GC_MINCHUNKS];
  void** references[GC_MAX_REFS][2];
};

void mun_gc_init(mun_alloc* alloc);
void mun_gc_add_ref(mun_alloc* alloc, void* ref);
void mun_gc_del_ref(mun_alloc* alloc, void* ref);
void mun_gc_minor(mun_alloc* alloc);
void mun_gc_major(mun_alloc* alloc);

void* mun_gc_alloc(mun_alloc* alloc, size_t size);

#ifdef MUN_DEBUG
void mun_gc_print_minor(mun_alloc* alloc);
void mun_gc_print_major(mun_alloc* alloc);
void mun_gc_print_refs(mun_alloc* alloc);
#endif

HEADER_END

#endif