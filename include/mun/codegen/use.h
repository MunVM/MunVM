#ifndef MUN_USE_H
#define MUN_USE_H

#include "../common.h"

HEADER_BEGIN

#include "../location.h"

typedef struct _use_position{
  word pos;
  location* slot;
  location* hint;
  struct _use_position* next;
} use_position;

MUN_INLINE void
use_position_init(use_position* use, word pos, use_position* next, location* loc){
  use->pos = pos;
  use->next = next;
  use->slot = loc;
  use->hint = NULL;
}

MUN_INLINE use_position*
use_position_new(word pos, use_position* next, location* slot){
  use_position* use = malloc(sizeof(use_position));
  use_position_init(use, pos, next, slot);
  return use;
}

MUN_INLINE bool
use_position_has_hint(use_position* use){
  return use->hint != NULL &&
         !loc_is_unallocated(*use->hint);
}

typedef struct _use_interval{
  word start;
  word end;
  struct _use_interval* next;
} use_interval;

MUN_INLINE void
use_interval_init(use_interval* interval, word start, word end, use_interval* next){
  interval->start = start;
  interval->end = end;
  interval->next = next;
}

MUN_INLINE use_interval*
use_interval_new(word start, word end, use_interval* next){
  use_interval* interval = malloc(sizeof(use_interval));
  use_interval_init(interval, start, end, next);
  return interval;
}

MUN_INLINE bool
use_interval_contains(use_interval* interval, word pos){
  return interval->start <= pos &&
         pos < interval->end;
}

MUN_INLINE word
use_interval_intersect(use_interval* interval, use_interval* other){
  if(interval->start <= other->start){
    if(other->start < interval->end) return other->start;
  } else if(interval->start < other->end) return interval->start;
  return kIllegalPosition;
}

HEADER_END

#endif