#ifndef MUN_STACK_FRAME_H
#define MUN_STACK_FRAME_H

#include "common.h"

#if defined(ARCH_IS_X64)
#include "x64/frame.h"
#else
#error "Unsupported CPU architecture"
#endif

#endif