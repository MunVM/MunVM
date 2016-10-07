#ifndef MUN_ITERATOR_H
#define MUN_ITERATOR_H

#include "common.h"

HEADER_BEGIN

typedef struct _iterator{
  bool (*done)(struct _iterator*);
  void (*advance)(struct _iterator*);
  void* (*current)(struct _iterator*);
} iterator;

#define FOREACH(iter) \
  for(iterator* it = ((iterator*) iter); \
      !it->done(it); \
      it->advance(it))

#define CURRENT it->current(it)

HEADER_END

#endif