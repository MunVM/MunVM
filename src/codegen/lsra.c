#include <mun/codegen/regalloc.h>

typedef struct _use_position {
  struct _use_position* next;
  word pos;
  location* slot;
  location* hint;
} use_position;

MUN_INLINE use_position*
use_position_new(word pos, use_position* next, location* slot) {
  use_position* use = malloc(sizeof(use_position));
  use->hint = NULL;
  use->slot = slot;
  use->next = next;
  use->pos = pos;
  return use;
}

#define HAS_HINT(use) ((use->hint != NULL) && !loc_is_unallocated(*use->hint))

typedef struct _use_interval {
  struct _use_interval* next;
  word start;
  word end;
} use_interval;

MUN_INLINE use_interval*
use_interval_new(word start, word end, use_interval* next) {
  use_interval* interval = malloc(sizeof(use_interval));
  interval->start = start;
  interval->end = end;
  interval->next = next;
  return interval;
}

#define CONTAINS(interval, pos) ((interval->start <= pos) && (pos < interval->end))

static word
use_interval_intersect(use_interval* interval, use_interval* other) {
  if (interval->start <= other->start) {
    if (other->start < interval->end) return other->start;
  } else if (interval->start < other->end) {
    return interval->start;
  }
  return kIllegalPosition;
}

typedef struct {
  use_interval* first_pending;
  use_position* first_register;
  use_position* first_beneficial;
  use_position* first_hinted;
} alloc_finger;

static void
alloc_finger_init(alloc_finger* finger) {
  finger->first_pending = NULL;
  finger->first_register = finger->first_beneficial = finger->first_hinted = NULL;
}

struct _live_range {
  word vreg;
  representation rep;
  location assigned;
  location spill;
  use_position* uses;
  use_interval* first_interval;
  use_interval* last_interval;
  live_range* next;
  alloc_finger finger;
};

static live_range*
live_range_new(word vreg, representation rep, use_position* uses, use_interval* first_interval, use_interval* last_interval, live_range* next){
  live_range* range = malloc(sizeof(live_range));
  range->vreg = vreg;
  range->rep = rep;
  range->assigned = kInvalidLocation;
  range->spill = kInvalidLocation;
  range->uses = uses;
  range->first_interval = first_interval;
  range->last_interval = last_interval;
  range->next = next;
  alloc_finger_init(&range->finger);
  return range;
}

static void
alloc_finger_initialize(alloc_finger* finger, live_range* range) {
  finger->first_pending = range->first_interval;
  finger->first_register = finger->first_beneficial = finger->first_hinted = range->uses;
}

#define FINGER(range) alloc_finger_initialize(&(range)->finger, range)
#define START(range) range->first_interval->start
#define END(range) range->last_interval->end
#define FIRST_PENDING(range) range->finger.first_pending
#define CAN_COVER(range, pos) ((START(range) <= pos) && (pos < END(range)))

static bool
alloc_finger_advance(alloc_finger* finger, word start) {
  use_interval* a = finger->first_pending;
  while (a != NULL && a->end <= start) a = a->next;
  return (finger->first_pending = a) != NULL;
}

static location
alloc_finger_first_hint(alloc_finger* finger) {
  use_position* use = finger->first_hinted;
  while (use != NULL) {
    if (HAS_HINT(use)) return *use->hint;
    use = use->next;
  }
  return kInvalidLocation;
}

static use_position*
first_use_after(use_position* use, word after) {
  while ((use != NULL) && (use->pos < after)) use = use->next;
  return use;
}

static use_position*
alloc_finger_first_register(alloc_finger* finger, word after) {
  for (use_position* use = first_use_after(finger->first_register, after);
       use != NULL;
       use = use->next) {
    location* slot = use->slot;
    if (loc_is_unallocated(*slot) &&
        ((loc_get_policy(*slot) == kRequiresRegister) ||
         (loc_get_policy(*slot) == kRequiresFpuRegister))) {
      return (finger->first_register = use);
    }
  }
  return NULL;
}

static use_position*
alloc_finger_first_beneficial(alloc_finger* finger, word after){
  for(use_position* use = first_use_after(finger->first_beneficial, after);
      use != NULL;
      use = use->next){
    location* slot = use->slot;
    if(loc_is_unallocated(*slot) && loc_is_register_beneficial(*slot)){
      return (finger->first_beneficial = use);
    }
  }
  return NULL;
}

static use_position*
alloc_finger_first_interfering(alloc_finger* finger, word after){
  if((after & 1) == 1) after += 1;
  return alloc_finger_first_register(finger, after);
}

static void
alloc_finger_update(alloc_finger* finger, word after){
  if((finger->first_register != NULL) &&
      (finger->first_register->pos >= after)){
    finger->first_register = NULL;
  }

  if((finger->first_beneficial != NULL) &&
      (finger->first_beneficial->pos < after)){
    finger->first_beneficial = NULL;
  }
}

static use_position*
split_lists(use_position** head, word split_pos, bool start){
  use_position* last_before_split = NULL;
  use_position* pos = *head;

  if(start){
    while((pos != NULL) && (pos->pos < split_pos)){
      last_before_split = pos;
      pos = pos->next;
    }
  } else{
    while((pos != NULL) && (pos->pos <= split_pos)){
      last_before_split = pos;
      pos = pos->next;
    }
  }

  if(last_before_split == NULL){
    *head = NULL;
  } else{
    last_before_split->next = NULL;
  }

  return pos;
}

static live_range*
live_range_split(live_range* range, word pos){
  if(START(range) == pos) return range;

  use_interval* interval = range->finger.first_pending;
  if(interval == NULL){
    FINGER(range);
    interval = range->finger.first_pending;
  }

  if(pos <= interval->start) interval = range->first_interval;

  use_interval* last_before_split = NULL;
  while(interval->end <= pos){
    last_before_split = interval;
    interval = interval->next;
  }

  bool split_at_start = (interval->start == pos);

  use_interval* first_after_split = interval;
  if(!split_at_start && CONTAINS(interval, pos)){
    first_after_split = use_interval_new(pos, interval->end, interval->next);
    interval->end = pos;
    interval->next = first_after_split;
    last_before_split = interval;
  }

  use_position* first_use_after_split = split_lists(&range->uses, pos, split_at_start);
  use_interval* last_use_interval = last_before_split == range->last_interval ?
                                    first_after_split :
                                    range->last_interval;
  range->next = live_range_new(range->vreg, range->rep, first_use_after_split, first_after_split, last_use_interval, range->next);
  range->last_interval = last_before_split;
  range->last_interval->next = NULL;
  if(first_use_after_split != NULL){
    alloc_finger_update(&range->finger, first_use_after_split->pos);
  }
  return range->next;
}

static bool
live_range_contains(live_range* range, word pos){
  if(!CAN_COVER(range, pos)) return FALSE;

  for(use_interval* interval = range->first_interval;
      interval != NULL;
      interval = interval->next){
    if(CONTAINS(interval, pos)) return TRUE;
  }

  return FALSE;
}