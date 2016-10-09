#ifndef MUN_RANGE_H
#define MUN_RANGE_H

#include "../common.h"

HEADER_BEGIN

#include "use.h"

typedef struct _live_range live_range;
typedef struct _alloc_finger alloc_finger;

location alloc_finger_first_hint(alloc_finger* finger);
use_position* alloc_finger_first_interfering(alloc_finger* finger, word after);
use_position* alloc_finger_first_use(alloc_finger* finger, word after);
use_position* alloc_finger_first_beneficial(alloc_finger* finger, word after);
use_interval* alloc_finger_first_pending(alloc_finger* finger);
void alloc_finger_init(alloc_finger* finger);
void alloc_finger_initialize(alloc_finger* finger, live_range* range);
void alloc_finger_update(alloc_finger* finger, word first_use_after_split);
bool alloc_finger_advance(alloc_finger* finger, word start);

struct _live_range{
  word vreg;
  representation rep;
  location assigned;
  location spill;
  use_position* uses;
  use_interval* first_interval;
  use_interval* last_interval;
  live_range* next;
  alloc_finger* finger;
};

MUN_INLINE void
live_range_init(live_range* self, word vreg, representation rep){
  self->vreg = vreg;
  self->rep = rep;
  self->assigned = kInvalidLocation;
  self->spill = kInvalidLocation;
  self->uses = NULL;
  self->first_interval = NULL;
  self->last_interval = NULL;
  self->next = NULL;
  self->finger = malloc(sizeof(alloc_finger));
  alloc_finger_init(self->finger);
}

#define START(range) range->first_interval->start
#define END(range) range->last_interval->end

void live_range_define(live_range* range, word pos);
void live_range_add_interval(live_range* range, word start, word end);

live_range* live_range_split(live_range* range, word pos);

use_position* live_range_add_use(live_range* range, word pos, location* slot);

MUN_INLINE void
live_range_add_hinted(live_range* range, word pos, location* slot, location* hint){
  live_range_add_use(range, pos, slot)->hint = hint;
}

bool live_range_contains(live_range* range, word pos);

#define CONTAINS(range, pos) live_range_contains(range, pos)
#define CAN_COVER(range, pos) ((START(range) <= pos) && (pos < END(range)))

HEADER_END

#endif