cmake_minimum_required(VERSION 3.3)
project(Mun)

include_directories(include)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99")
add_executable(Mun src/main.c include/mun/common.h include/mun/alloc.h src/alloc.c include/mun/type.h include/mun/sstream.h src/sstream.c include/mun/table.h include/mun/function.h src/table.c src/type.c include/mun/iterator.h include/mun/ast.h include/mun/array.h src/array.c src/ast.c include/mun/asm.h include/mun/x64/core.h src/x64/core.c src/asm.c include/mun/bitfield.h include/mun/location.h src/location.c include/mun/codegen/instruction.h src/codegen/instructions.c src/codegen/definitions.h src/codegen/definitions.c src/codegen/all.h src/codegen/x64/compile.c src/codegen/x64/compile.h include/mun/codegen/graph.h src/codegen/graph.c src/codegen/blocks.c src/codegen/blocks.h include/mun/scope.h include/mun/stack_frame.h include/mun/x64/frame.h src/scope.c include/mun/parser.h src/parser.c include/mun/codegen/regalloc.h include/mun/codegen/liveness.h include/mun/bitvec.h src/mun.c src/codegen/liveness.c src/bitvec.c)