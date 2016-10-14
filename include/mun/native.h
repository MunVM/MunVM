#ifndef MUN_NATIVE_H
#define MUN_NATIVE_H

#include "common.h"

HEADER_BEGIN

#include "type.h"

typedef struct _mun_native_args* mun_native_args;
typedef void (*mun_native_function)(mun_native_args args);

typedef struct _native_arguments{
  instance** argv;
  instance** retval;
  word argc;
} native_arguments;

#define ARGC_OFFSET offsetof(native_arguments, argc)
#define ARGV_OFFSET offsetof(native_arguments, argv)
#define RETVAL_OFFSET offsetof(native_arguments, retval)

MUN_INLINE instance*
native_arguments_at(native_arguments* args, word index){
  instance** arg = &(args->argv[-index]);
  return *arg;
}

#define RETURN(args, val) *(args)->retval = val

HEADER_END

#endif