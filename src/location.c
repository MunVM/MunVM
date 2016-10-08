#include <mun/location.h>
#include <mun/stack_frame.h>
#include <mun/codegen/instruction.h>

enum {
  kBitsForKind = 4,
  kBitsForPayload = sizeof(word) * 8 - kBitsForKind,
  kBitsForBaseReg = 5,
  kBitsForStackIndex = kBitsForPayload - kBitsForBaseReg,
};

static const uword kLocationMask = 0x3;
static const word kStackIndexBias = 1 << ((kBitsForPayload - 5) - 1);

static const bit_field kPayloadField = {
    kBitsForKind,
    kBitsForPayload
};

static const bit_field kKindField = {
    0,
    kBitsForKind
};

static const bit_field kPolicyField = {
    0, 3
};

static const bit_field kStackSlotBaseField = {
    0,
    kBitsForBaseReg
};

static const bit_field kStackIndexField = {
    kBitsForBaseReg,
    kBitsForStackIndex
};

location_kind
loc_get_kind(location loc) {
  return ((location_kind) bit_field_decode(&kKindField, loc));
}

location_policy
loc_get_policy(location loc) {
  return ((location_policy) bit_field_decode(&kPolicyField, loc_get_payload(loc)));
}

uword
loc_get_payload(location loc) {
  return ((uword) bit_field_decode(&kPayloadField, loc));
}

void
loc_init(location* loc) {
  *loc = kInvalidLocation;
}

MUN_INLINE uword
unallocated(location_policy policy) {
  uword payload = ((uword) bit_field_encode(&kPolicyField, policy));
  return ((uword) bit_field_encode(&kKindField, kUnallocated) | bit_field_encode(&kPayloadField, payload));
}

MUN_INLINE uword
allocated(location_kind kind, uword payload) {
  return ((uword) bit_field_encode(&kKindField, kind) | bit_field_encode(&kPayloadField, payload));
}

void
loc_init_s(location* loc) {
  *loc = unallocated(kSameAsFirstInput);
}

void
loc_init_a(location* loc) {
  *loc = unallocated(kAny);
}

void
loc_init_c(location* loc, constant_instr* c){
  *loc = ((uword) c) | kConstant;
}

void
loc_init_r(location* loc, asm_register reg) {
  *loc = allocated(kRegister, reg);
}

void
loc_init_x(location* loc, asm_fpu_register reg) {
  *loc = allocated(kFpuRegister, reg);
}

MUN_INLINE uword
encode_stack_index(word index) {
  return ((uword) kStackIndexBias + index);
}

void
loc_init_z(location* loc, word slot) {
  uword payload = ((uword) bit_field_encode(&kStackSlotBaseField, FPREG) |
                   bit_field_encode(&kStackIndexField, encode_stack_index(slot)));
  *loc = allocated(kStackSlot, payload);
}

void
loc_init_d(location* loc, word slot) {
  uword payload = ((uword) bit_field_encode(&kStackSlotBaseField, FPREG) |
                   bit_field_encode(&kStackIndexField, encode_stack_index(slot)));
  *loc = allocated(kDoubleStackSlot, payload);
}

void
loc_init_zr(location* loc, word slot, asm_register base) {
  uword payload = ((uword) bit_field_encode(&kStackSlotBaseField, base) |
                   bit_field_encode(&kStackIndexField, encode_stack_index(slot)));
  *loc = allocated(kStackSlot, payload);
}

bool
loc_is_constant(location loc) {
  return (loc & kLocationMask) == kConstant;
}

bool
loc_is_invalid(location loc) {
  return loc == kInvalidLocation;
}

void
loc_hint_pr(location* loc) {
  *loc = unallocated(kPrefersRegister);
}

void
loc_hint_rr(location* loc) {
  *loc = unallocated(kRequiresRegister);
}

void
loc_hint_rx(location* loc) {
  *loc = unallocated(kRequiresFpuRegister);
}

word
loc_get_stack_slot(location loc) {
  return bit_field_decode(&kStackIndexField, loc_get_payload(loc)) - kStackIndexBias;
}

asm_register
loc_get_stack_register(location loc){
  return ((asm_register) bit_field_decode(&kStackSlotBaseField, loc_get_payload(loc)));
}

constant_instr*
loc_get_constant(location loc){
  return ((constant_instr*) (loc & ~kLocationMask));
}

void
loc_encode_stack_address(location loc, asm_address* addr){
  word index = loc_get_stack_slot(loc);
  asm_register base = loc_get_stack_register(loc);

  if(base == FPREG){
    if(index < 0){
      word offset = (kParamEndSlotFromFp - index) * kWordSize;
      asm_addr_init_r(addr, base, offset);
    } else{
      word offset = (kFirstLocalSlotFromFp - index) * kWordSize;
      asm_addr_init_r(addr, base, offset);
    }
  } else{
    asm_addr_init_r(addr, base, index * kWordSize);
  }
}

location_summary*
loc_summary_new(word in_count, word temp_count, bool call) {
  location_summary* summary = malloc(sizeof(location_summary));
  summary->inputs = malloc(sizeof(location) * in_count);
  summary->inputs_len = in_count;
  summary->temps = malloc(sizeof(location) * temp_count);
  summary->temps_len = temp_count;
  summary->contains_call = call;
  return summary;
}

location_summary*
loc_summary_make(word in_count, location out, bool call) {
  location_summary* summary = loc_summary_new(in_count, 0, call);
  for (word i = 0; i < in_count; i++) {
    loc_hint_rr(&summary->inputs[i]);
  }
  summary->output = out;
  return summary;
}