#ifndef MUN_AST_H
#define MUN_AST_H

#include "common.h"

HEADER_BEGIN

#include "array.h"
#include "type.h"
#include "scope.h"

typedef enum{
  kIllegalNode = -1,
  kReturnNode = 1 << 0,
  kLiteralNode = 1 << 1,
  kSequenceNode = 1 << 2,
  kBinaryOpNode = 1 << 3,
  kLoadLocalNode = 1 << 4,
  kStoreLocalNode = 1 << 5,
  kStaticCallNode = 1 << 6,
  kNativeCallNode = 1 << 7,
  kLoadIndexedNode = 1 << 8,
  kStoreIndexedNode = 1 << 9,
} ast_node_type;

typedef enum{
  kAdd = 1 << 0,
  kSubtract = 1 << 1,
  kMultiply = 1 << 2,
  kDivide = 1 << 3,
} binary_operation;

typedef struct _ast_node ast_node;
struct _ast_node{
  ast_node_type type;
  union{
    struct{
      array* children; // ast_node*
      local_scope* scope;
    } sequence;
    struct{
      instance* value;
    } literal;
    struct{
      ast_node* value;
    } ret;
    struct{
      ast_node* left;
      ast_node* right;
      binary_operation oper;
    } binary_op;
    struct{
      local_variable* local;
    } load_local;
    struct{
      local_variable* local;
      ast_node* value;
    } store_local;
    struct{
      ast_node* table;
      ast_node* index;
    } load_indexed;
    struct{
      ast_node* table;
      ast_node* index;
      ast_node* value;
    } store_indexed;
    struct{
      struct _function* func;
    } static_call;
    struct{
      struct _function* func;
    } native_call;
  } as;
};

ast_node* return_node_new(mun_alloc* alloc, ast_node* value);
ast_node* literal_node_new(mun_alloc* alloc, instance* value);
ast_node* sequence_node_new(mun_alloc* alloc);
ast_node* binary_op_node_new(mun_alloc* alloc, ast_node* left, ast_node* right, binary_operation oper);
ast_node* load_local_node_new(mun_alloc* alloc, local_variable* local);
ast_node* store_local_node_new(mun_alloc* alloc, local_variable* local, ast_node* value);
ast_node* load_indexed_node_new(mun_alloc* alloc, ast_node* array, ast_node* index);
ast_node* store_indexed_node_new(mun_alloc* alloc, ast_node* array, ast_node* index, ast_node* value);
ast_node* native_call_node_new(mun_alloc* alloc, struct _function* func);
ast_node* static_call_node_new(mun_alloc* alloc, struct _function* func);

MUN_INLINE void
sequence_node_append(ast_node* seq, ast_node* child){
  array_add(seq->as.sequence.children, child);
}

typedef struct _ast_node_visitor{
  void (*visit_sequence)(struct _ast_node_visitor*, ast_node*);
  void (*visit_binary_op)(struct _ast_node_visitor*, ast_node*);
  void (*visit_literal)(struct _ast_node_visitor*, ast_node*);
  void (*visit_return)(struct _ast_node_visitor*, ast_node*);
  void (*visit_store_local)(struct _ast_node_visitor*, ast_node*);
  void (*visit_load_local)(struct _ast_node_visitor*, ast_node*);
} ast_node_visitor;

void ast_visit(ast_node_visitor* vis, ast_node* node);

HEADER_END

#endif