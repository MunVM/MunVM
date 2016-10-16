#include <mun/array.h>
#include "all.h"

NAME(return);
INPUT_COUNT(return, 1);
INPUT_AT(return, value);

DEFINE(Return){
    &return_set_input_at, // set_input_at
    &return_compile, // compile
    NULL, // get_representation
    NULL, // get_input_representation
    &return_make_location_summary, // make_location_summary
    NULL, // argument_at
    NULL, // successor_at
    NULL, // argument_count
    &return_input_count, // input_count
    NULL, // successor_count
    &return_input_at, // input_at
    &return_name, // name
};

NAME(constant);
INPUT_COUNT(constant, 0);

instance*
constant_instr_get_value(instruction* instr){
  return to_constant_instr(instr)->value;
}

DEFINE(Constant){
    NULL, // set_input_at
    &constant_compile, // compile
    NULL, // get_representation
    NULL, // get_input_representation
    &constant_make_location_summary, // make_location_summary
    NULL, // argument_at
    NULL, // successor_at
    NULL, // argument_count
    &constant_input_count, // input_count
    NULL, // successor_count
    NULL, // input_at
    &constant_name, // name
};

NAME(binary_op);
INPUT_COUNT(binary_op, 2);
INPUT_AT(binary_op, inputs[index]);

DEFINE(BinaryOp){
    &binary_op_set_input_at, // set_input_at
    &binary_op_compile, // compile
    NULL, // get_representation
    NULL, // get_input_representation
    &binary_op_make_location_summary, // make_location_summary
    NULL, // argument_at
    NULL, // successor_at
    NULL, // argument_count
    &binary_op_input_count, // input_count
    NULL, // successor_count
    &binary_op_input_at, // input_at
    &binary_op_name, // name
};

NAME(box);
INPUT_COUNT(box, 1);
INPUT_AT(box, value);

DEFINE(Box){
    &box_set_input_at, // set_input_at
    NULL, // compile
    NULL, // get_representation
    NULL, // get_input_representation
    NULL, // make_location_summary
    NULL, // argument_at
    NULL, // successor_at
    NULL, // argument_count
    &box_input_count, // input_count
    NULL, // successor_count
    &box_input_at, // input_at
    &box_name, // name
};

NAME(unbox);
INPUT_COUNT(unbox, 1);
INPUT_AT(unbox, value);

DEFINE(Unbox){
    &unbox_set_input_at, // set_input_at
    NULL, // compile
    NULL, // get_representation
    NULL, // get_input_representation
    NULL, // make_location_summary
    NULL, // argument_at
    NULL, // successor_at
    NULL, // argument_count
    &unbox_input_count, // input_count
    NULL, // successor_count
    &unbox_input_at, // input_at
    &unbox_name, // name
};

NAME(store_local);
INPUT_COUNT(store_local, 1);
INPUT_AT(store_local, value);

DEFINE(StoreLocal){
    &store_local_set_input_at, // set_input_at
    &store_local_compile, // compile
    NULL, // get_representation
    NULL, // get_input_representation
    &store_local_make_location_summary, // make_location_summary
    NULL, // argument_at
    NULL, // successor_at
    NULL, // argument_count
    &store_local_input_count, // input_count
    NULL, // successor_count
    &store_local_input_at, // input_at
    &store_local_name, // name
};

NAME(load_local);
INPUT_COUNT(load_local, 0);

DEFINE(LoadLocal){
    NULL, // set_input_at
    NULL, // compile
    NULL, // get_representation
    NULL, // get_input_representation
    NULL, // make_location_summary
    NULL, // argument_at
    NULL, // successor_at
    NULL, // argument_count
    &load_local_input_count, // input_count
    NULL, // successor_count
    NULL, // input_at
    &load_local_name, // name
};

NAME(phi);
INPUT_COUNT(phi, to_phi_instr(instr)->inputs.size);
INPUT_AT(phi, inputs.data[index]);

DEFINE(Phi){
    &phi_set_input_at, // set_input_at
    NULL, // compile
    NULL, // get_representation
    NULL, // get_input_representation
    NULL, // make_location_summary
    NULL, // argument_at
    NULL, // successor_at
    NULL, // argument_count
    &phi_input_count, // input_count
    NULL, // successor_count
    &phi_input_at, // input_at
    &phi_name, // name
};

NAME(native_call);
INPUT_COUNT(native_call, 0x0);

DEFINE(NativeCall){
    NULL, // set_input_at
    &native_call_compile, // compile
    NULL, // get_representation
    NULL, // get_input_representation
    NULL, // make_location_summary
    NULL, // argument_at
    NULL, // successor_at
    NULL, // argument_count
    &native_call_input_count, // input_count
    NULL, // successor_count
    NULL, // input_at
    &native_call_name, // name
};