#ifndef MUN_COMMON_H
#define MUN_COMMON_H

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>

#ifdef __cplusplus
#define HEADER_BEGIN extern "C"{
#define HEADER_END };
#else
#define HEADER_BEGIN
#define HEADER_END
#endif

HEADER_BEGIN

#if defined(_MSC_VER)
#define MUN_INLINE static __inline
#else
#define MUN_INLINE static inline
#endif

typedef intptr_t word;
typedef uintptr_t uword;

typedef uintptr_t location;

static const size_t kWordSize = sizeof(word);
static const int kBitsPerByte = 8;
static const int kBitsPerWord = sizeof(word) * 8;
static const int kIllegalPosition = -1;
static const int kNoVirtualRegister = -1;
static const int kTempVirtualRegister = -2;
static const int kMaxPosition = 0x7FFFFFFF;

typedef struct _mun_alloc mun_alloc;

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#if defined(_M_X64) || defined(__x86_64__)
#define ARCH_IS_X64 1
#elif defined(_M_IX86) || defined(__i386__)
#define ARCH_IS_X32 1
#else
#error "Cannot determine CPU architecture"
#endif

#if defined(_WIN32)
#define TARGET_IS_WINDOWS 1
#elif defined(__linux__) || defined(__FreeBSD__)
#define TARGET_IS_LINUX 1
#else
#error "Cannot determine Operating System"
#endif

#ifndef container_of
#define container_of(ptr_, type_, member_)({ \
  const typeof(((type_*) 0)->member_)* __mbptr = ((void*) ptr_); \
  (type_*)((char*) __mbptr - offsetof(type_, member_)); \
})
#endif

#ifndef MUN_DEBUG
#define MUN_DEBUG 0
#endif

MUN_INLINE word*
wdup(word value){
  word* nword = malloc(sizeof(word));
  memcpy(nword, &value, sizeof(word));
  return nword;
}

//TODO: Remove In Production
extern mun_alloc* GC;

HEADER_END

#endif