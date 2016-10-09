#ifndef MUN_REGALLOC_H
#define MUN_REGALLOC_H

#if !defined(MUN_USE_LSRA)
#define MUN_USE_LSRA 1
#endif

#if defined(MUN_USE_LSRA)
#include "lsra.h"
#else
#error "Please define a register allocator"
#endif

#endif