#include <mun/codegen/range.h>

struct _alloc_finger {
  use_interval* first_pending;
  use_position* first_use;
  use_position* first_beneficial;
  use_position* first_hinted;
};

void
alloc_finger_initialize(alloc_finger* finger, live_range* range) {
  //TODO: Initialize
}

location
alloc_finger_first_hint(alloc_finger* finger) {
  use_position* use = finger->first_hinted;
  while (use != NULL) {
    if (use_position_has_hint(use)) return *use->hint;
    use = use->next;
  }
  return kInvalidLocation;
}

static use_position*
first_use_after(use_position* use, word after) {
  while ((use != NULL) && (use->pos < after)) use = use->next;
  return use;
}

use_position*
alloc_finger_first_use(alloc_finger* finger, word after) {
  for (use_position* use = first_use_after(finger->first_use, after);
       use != NULL;
       use = use->next) {
    location* slot = use->slot;
    if (loc_is_unallocated(*slot) &&
        ((loc_get_policy(*slot) == kRequiresRegister) ||
         (loc_get_policy(*slot) == kRequiresFpuRegister))) {
      return (finger->first_use = use);
    }
  }
  return NULL;
}

use_position*
alloc_finger_first_beneficial(alloc_finger* finger, word after) {
  for (use_position* use = first_use_after(finger->first_beneficial, after);
       use != NULL;
       use = use->next) {
    location* slot = use->slot;
    if (loc_is_unallocated(*slot) && loc_is_register_beneficial(*slot)) {
      return (finger->first_beneficial = use);
    }
  }
  return NULL;
}

use_position*
alloc_finger_first_interfering(alloc_finger* finger, word after) {
  if ((after & 1) == 1) after += 1;
  return alloc_finger_first_use(finger, after);
}

void
alloc_finger_init(alloc_finger* finger){
  finger->first_pending = NULL;
  finger->first_use = finger->first_beneficial = finger->first_hinted = NULL;
}

void
alloc_finger_update(alloc_finger* finger, word first_use_after_split) {
  if ((finger->first_use != NULL) &&
      (finger->first_use->pos >= first_use_after_split)) {
    finger->first_use = NULL;
  }

  if ((finger->first_beneficial != NULL) &&
      (finger->first_beneficial->pos < first_use_after_split)) {
    finger->first_beneficial = NULL;
  }
}

use_interval*
alloc_finger_first_pending(alloc_finger* finger){
  return finger->first_pending;
}

bool
alloc_finger_advance(alloc_finger* finger, word start){
  use_interval* a = finger->first_pending;
  while(a != NULL && a->end <= start) a = a->next;
  return (finger->first_pending = a) != NULL;
}