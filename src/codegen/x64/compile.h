#ifndef MUN_COMPILE_H
#define MUN_COMPILE_H

#if !defined(MUN_ALL_H)
#error "Please #include <mun/all.h> directly"
#endif

#define DECLARE_COMPILE(Name) \
  void Name##_compile(instruction*, asm_buff*); \
  location_summary* Name##_make_location_summary(instruction*);
DECLARE_COMPILE(constant);
DECLARE_COMPILE(return);
DECLARE_COMPILE(native_call);
DECLARE_COMPILE(store_local);
DECLARE_COMPILE(binary_op);
#undef DECLARE_COMPILE

#endif