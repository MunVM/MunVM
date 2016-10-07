#include <mun/alloc.h>

#define WITH_HEADER(size) ((size) + sizeof(int))
#define WITHOUT_HEADER(size) ((size) - sizeof(int))
#define FLAGS(chunk) (*((unsigned int*)(chunk)))
#define CHUNK_SIZE(chunk) (FLAGS((chunk)) & (~3))
#define MARK(chunk, c) ((FLAGS(chunk)) = CHUNK_SIZE(chunk)|c)
#define PTR(ofs) ((ofs) / sizeof(void*))
#define BITS(chunk) (WITH_HEADER(chunk))
#define BITS_AT(chunk, idx) ((((void**)(BITS(chunk)) + (idx) * sizeof(void*))))
#define MEM(ptr) (!(((unsigned int)(ptr)) & 1))
#define POINTS_MINOR(ptr) (((byte*)(ptr)) >= &(gc)->minor_heap[0] && ((byte*)(ptr)) < &(gc)->minor_heap[GC_MINHEAP_SIZE])
#define POINTS_MAJOR(ptr) (((byte*)(ptr)) >= &(gc)->major_heap[0] && ((byte*)(ptr)) < &(gc)->major_heap[GC_MAJHEAP_SIZE])
#define REF_PTR_MIN(ptr) (MEM((ptr)) && POINTS_MINOR((ptr)))
#define REF_PTR_MAJ(ptr) (MEM((ptr)) && POINTS_MAJOR((ptr)))
#define MINOR_CHUNK(ptr) (((((byte*)(ptr) - &(gc)->minor_heap[0]) / GC_MINCHUNK_SIZE) * GC_MINCHUNK_SIZE) + &(gc)->minor_heap[0])
#define MARKED(chunk) (FLAGS(chunk) & 1)
#define CHUNK_FLAGS(chunk) (FLAGS((chunk)) & (3))
#define ALIGN(ptr) (((ptr) + 3) & ~3)
#define CHUNK_AT(idx) (&(gc)->minor_heap[(idx) * GC_MINCHUNK_SIZE])
#define CHUNK_OFFSET(chunk) ((((chunk)) - &(gc)->minor_heap[0]) / GC_MINCHUNK_SIZE)

typedef enum{
  GC_FREE = 0,
  GC_WHITE,
  GC_BLACK,
  GC_GRAY
} gc_color;

void
mun_gc_init(mun_alloc* gc){
  gc->free_chunk = 0x0;
  gc->refs_count = 0x0;
  FLAGS(&gc->major_heap[0]) = GC_MAJHEAP_SIZE;
  memset(gc->backpatch, 0, sizeof(gc->backpatch));
}

static void
set_ref_block(mun_alloc* gc, int index, void* start, void* end){
  gc->references[index][0] = start;
  gc->references[index][1] = end;
}

static byte*
find_major_chunk(mun_alloc* gc, byte* ptr){
  byte* curr;
  for(curr = &gc->major_heap[0];
      !(ptr >= curr && (ptr < (curr + CHUNK_SIZE(curr)))) && curr < &gc->major_heap[GC_MAJHEAP_SIZE];
      curr += CHUNK_SIZE(curr));
  if(curr < &gc->major_heap[GC_MAJHEAP_SIZE]) return curr;
  return NULL;
}

static void
mark_major_chunk(mun_alloc* gc, byte* ptr){
  if(CHUNK_FLAGS(ptr) != GC_BLACK){
    MARK(ptr, CHUNK_FLAGS(ptr)|GC_BLACK);
    for(int i = 0; i < PTR(CHUNK_SIZE(ptr)); i++){
      if(REF_PTR_MAJ(*BITS_AT(ptr, i))){
        mark_major_chunk(gc, find_major_chunk(gc, *BITS_AT(ptr, i)));
      }
    }
  }
}

static void* major_alloc(mun_alloc* gc, size_t size);

static void*
minor_alloc(mun_alloc* gc, size_t size){
  if(size == 0) return NULL;
  size = WITH_HEADER(ALIGN(size));
  if(size > GC_MINCHUNK_SIZE) return major_alloc(gc, WITHOUT_HEADER(size));
  if(gc->free_chunk >= GC_MINCHUNKS){
    mun_gc_minor(gc);
    return minor_alloc(gc, WITHOUT_HEADER(size));
  }

  FLAGS(CHUNK_AT(gc->free_chunk)) = ((unsigned int) size);
  void* raw = BITS(CHUNK_AT(gc->free_chunk));
  gc->free_chunk++;
  return raw;
}

static void*
major_alloc(mun_alloc* gc, size_t size){
  if(size == 0) return NULL;
  size = WITH_HEADER(size);

  byte* curr;
  for(curr = &gc->major_heap[0];
      (CHUNK_FLAGS(curr) > GC_FREE && size > CHUNK_SIZE(curr));
      curr += CHUNK_SIZE(curr));

  if(curr >= &gc->major_heap[GC_MAJHEAP_SIZE]) return NULL;

  byte* free_chunk = curr;
  unsigned int prev_size = CHUNK_SIZE(free_chunk);
  FLAGS(free_chunk) = prev_size;
  MARK(free_chunk, GC_WHITE);
  byte* next_chunk = (curr + size);
  FLAGS(next_chunk) = ((unsigned int) (prev_size - size));
  return ((void*) WITH_HEADER(free_chunk));
}

static void
darken_chunk(mun_alloc* gc, byte* ptr){
  byte* curr = find_major_chunk(gc, ptr);
  if(curr != NULL && CHUNK_FLAGS(curr) == GC_WHITE) MARK(curr, GC_GRAY);
}

static void
darken_roots(mun_alloc* gc){
  for(int i = 0; i < gc->refs_count; i++){
    for(void** ref = gc->references[i][0];
        ref < gc->references[i][1];
        ref++){
      if(ref != NULL) darken_chunk(gc, *ref);
    }
  }
}

static void
darken_major(mun_alloc* gc){
  byte* curr;
  for(curr = &gc->major_heap[0];
      curr != NULL && curr < &gc->major_heap[GC_MAJHEAP_SIZE];
      curr += CHUNK_SIZE(curr)){
    switch(CHUNK_FLAGS(curr)){
      case GC_GRAY: mark_major_chunk(gc, curr); break;
      default: break;
    }
  }
}

static void
mark_chunk(mun_alloc* gc, byte* ptr){
  MARK(ptr, 1);
  for(int i = 0; i < PTR(CHUNK_SIZE(ptr)); i++){
    if(REF_PTR_MIN(*BITS_AT(ptr, i))){
      byte* p = *BITS_AT(ptr, i);
      byte* ref = MINOR_CHUNK(p);
      if(!MARKED(ref)) mark_chunk(gc, ref);
    }
  }
}

static void
mark_minor(mun_alloc* gc){
  memset(gc->backpatch, 0, sizeof(gc->backpatch));
  for(int i = 0; i < gc->refs_count; i++){
    for(void** ref = gc->references[i][0];
        ref < gc->references[i][1];
        ref++){
      if(ref != NULL && REF_PTR_MIN(*ref)) mark_chunk(gc, MINOR_CHUNK(*ref));
    }
  }
}

static void
backpatch_chunk(mun_alloc* gc, byte* ptr){
  for(int i = 0; i < PTR(CHUNK_SIZE(ptr)); i++){
    if(REF_PTR_MIN(*BITS_AT(ptr, i))){
      byte* p = *BITS_AT(ptr, i);
      byte* ref = MINOR_CHUNK(p);
      if(MARKED(ref)){
        byte* new_ptr = ((byte*) gc->backpatch[CHUNK_OFFSET(ref)]);
        *BITS_AT(ptr, i) += (new_ptr - ref);
      }
    }
  }
}

static void
backpatch_refs(mun_alloc* gc){
  for(int i = 0; i < gc->refs_count; i++){
    for(void** ref = gc->references[i][0];
        ref < gc->references[i][1];
        ref++){
      if(ref != NULL && POINTS_MINOR(*ref)){
        byte* chunk = MINOR_CHUNK(*ref);
        if(MARKED(chunk)){
          byte* new_ptr = ((byte*) gc->backpatch[CHUNK_OFFSET(chunk)]);
          if(new_ptr != NULL) *ref += (new_ptr - chunk);
        }
      }
    }
  }
}

static void
copy_minor_heap(mun_alloc* gc){
  for(int i = 0; i < gc->free_chunk; i++){
    byte* chunk = CHUNK_AT(i);
    if(MARKED(chunk)){
      void* ptr = WITHOUT_HEADER(major_alloc(gc, WITHOUT_HEADER(CHUNK_SIZE(chunk))));
      gc->backpatch[i] = ptr;

      for(int j = 0; j < PTR(CHUNK_SIZE(chunk)); j++){
        if(REF_PTR_MIN(*BITS_AT(chunk, j))){
          void** p = BITS_AT(chunk, j);
          byte* ref_chunk = MINOR_CHUNK(*p);
          if(MARKED(ref_chunk)){
            mun_gc_add_ref(gc, *p);
          }
        }
      }
    }
  }

  backpatch_refs(gc);

  for(int i = 0; i < gc->refs_count; i++) backpatch_chunk(gc, CHUNK_AT(i));

  for(int i = 0; i < gc->refs_count; i++){
    byte* chunk = CHUNK_AT(i);
    if(MARKED(chunk)){
      byte* new_chunk = ((byte*) gc->backpatch[i]);
      memcpy(new_chunk, chunk, CHUNK_SIZE(chunk));
    }
  }
}

void
mun_gc_major(mun_alloc* gc){
  darken_roots(gc);
  darken_major(gc);

  for(byte* curr = &gc->major_heap[0];
      curr < &gc->major_heap[GC_MAJHEAP_SIZE];
      curr += CHUNK_SIZE(curr)){
    switch(CHUNK_FLAGS(curr)){
      case GC_WHITE: MARK(curr, GC_FREE); break;
      default: break;
    }
  }
}

void
mun_gc_minor(mun_alloc* gc){
  mark_minor(gc);
  copy_minor_heap(gc);
  gc->free_chunk = 0x0;
}

void*
mun_gc_alloc(mun_alloc* gc, size_t size){
  void* ptr = minor_alloc(gc, size);
  if(ptr == NULL) return major_alloc(gc, size);
  return ptr;
}

#define CHECK_CHUNK(begin, end)({ \
  if(begin == end){ \
    return; \
  } \
  if(begin > end){ \
    void** tmp = begin; \
    begin = end; \
    end = tmp; \
  } \
})

static void
add_ref(mun_alloc* gc, void** begin, void** end){
  CHECK_CHUNK(begin, end);
  for(int i = 0; i < gc->refs_count; i++){
    if(begin >= gc->references[i][0] && end <= gc->references[i][1]) return;
  }

  for(int i = 0; i < gc->refs_count; i++){
    if(gc->references[i][0] == 0){
      set_ref_block(gc, i, begin, end);
      return;
    }
  }

  set_ref_block(gc, gc->refs_count++, begin, end);
}

static void
remove_ref(mun_alloc* gc, void* begin, void* end){
  CHECK_CHUNK(begin, end);

  int i;
  for(i = 0; i < gc->refs_count; i++){
    if(((void**) begin) >= gc->references[i][0] && ((void**) end) <= gc->references[i][1]){
      gc->references[i][0] = 0x0;
      gc->references[i][1] = 0x0;
      return;
    }
  }

  if(((void**) begin) >= gc->references[i][0] && ((void**) end) <= gc->references[i][1]){
    gc->refs_count--;
  }
}

void
mun_gc_add_ref(mun_alloc* gc, void* ref){
  add_ref(gc, ref, ref + sizeof(void*));
}

void
mun_gc_del_ref(mun_alloc* gc, void* ref){
  remove_ref(gc, ref, ref + sizeof(void*));
}