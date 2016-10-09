#include <mun/codegen/range.h>

use_position*
live_range_add_use(live_range* range, word pos, location* slot) {
  if (range->uses != NULL) {
    if ((range->uses->pos == pos) &&
        (range->uses->slot == slot)) {
      return range->uses;
    } else if(range->uses->pos < pos){
      use_position* insert_after = range->uses;
      while((insert_after->next != NULL) &&
            (insert_after->next->pos < pos)) insert_after = insert_after->next;
      use_position* insert_before = insert_after->next;
      while((insert_before != NULL) &&
            (insert_before->pos == pos)){
        if(insert_before->slot == slot) return insert_before;
      }

      return (insert_after->next = use_position_new(pos, insert_after->next, slot));
    }
  }

  return (range->uses = use_position_new(pos, range->uses, slot));
}

void
live_range_add_interval(live_range* range, word start, word end){
  if(range->first_interval != NULL){
    if(start > range->first_interval->start) return;
  } else if(start == range->first_interval->start){
    if(end <= range->first_interval->end) return;
    range->first_interval->end = end;
    return;
  } else if(end == range->first_interval->start){
    range->first_interval->start = start;
    return;
  }

  range->first_interval = use_interval_new(start, end, range->first_interval);
  if(range->last_interval == NULL) range->last_interval = range->first_interval;
}

void
live_range_define(live_range* range, word pos){
  if(range->first_interval == NULL){
    range->first_interval = range->last_interval = use_interval_new(pos, pos + 1, NULL);
  } else{
    range->first_interval->start = pos;
  }
}

MUN_INLINE live_range*
live_range_new(word vreg, representation rep, use_position* uses, use_interval* first_use_interval, use_interval* last_use_interval, live_range* next){
  live_range* range = malloc(sizeof(live_range));
  range->vreg = vreg;
  range->rep = rep;
  range->assigned = kInvalidLocation;
  range->spill = kInvalidLocation;
  range->uses = uses;
  range->first_interval = first_use_interval;
  range->last_interval = last_use_interval;
  range->next = next;
  range->finger = malloc(sizeof(alloc_finger));
  alloc_finger_init(range->finger);
  return range;
}

use_position*
split_lists(use_position** head, word split_pos, bool split_at_start){
  use_position* last_before_split = NULL;
  use_position* pos = *head;

  if(split_at_start){
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

live_range*
live_range_split(live_range* range, word pos){
  if(START(range) == pos) return range;

  use_interval* interval = alloc_finger_first_pending(range->finger);
  if(interval == NULL){
    alloc_finger_initialize(range->finger, range);
    interval = alloc_finger_first_pending(range->finger);
  }

  if(pos <= interval->start) interval = range->first_interval;

  use_interval* last_before_split = NULL;
  while(interval->end <= pos){
    last_before_split = interval;
    interval = interval->next;
  }

  bool split_at_start = (interval->start == pos);

  use_interval* first_after_split = interval;
  if(!split_at_start && use_interval_contains(interval, pos)){
    first_after_split = use_interval_new(pos, interval->end, interval->next);
    interval->end = pos;
    interval->next = first_after_split;
    last_before_split = interval;
  }

  use_position* first_use_after_split = split_lists(&range->uses, pos, split_at_start);
  use_interval* last_use_interval = (last_before_split == range->last_interval) ?
                                    first_after_split :
                                    range->last_interval;
  range->next = live_range_new(range->vreg, range->rep, first_use_after_split, first_after_split, last_use_interval, range->next);
  range->last_interval = last_before_split;
  range->last_interval->next = NULL;
  if(first_use_after_split != NULL){
    alloc_finger_update(range->finger, first_use_after_split->pos);
  }
  return range->next;
}

bool
live_range_contains(live_range* range, word pos){
  if(!CAN_COVER(range, pos)) return FALSE;

  for(use_interval* interval = range->first_interval;
      interval != NULL;
      interval = interval->next){
    if(use_interval_contains(interval, pos)) return TRUE;
  }

  return FALSE;
}