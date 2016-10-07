#include <mun/ast.h>
#include <mun/alloc.h>
#include <mun/function.h>

ast_node*
ast_node_new(mun_alloc* alloc, ast_node_type type){
  ast_node* node = mun_gc_alloc(alloc, sizeof(ast_node));
  node->type = type;
  return node;
}

ast_node*
return_node_new(mun_alloc* alloc, ast_node* value){
  ast_node* node = ast_node_new(alloc, kReturnNode);
  node->as.ret.value = value;
  return node;
}

ast_node*
literal_node_new(mun_alloc* alloc, instance* value){
  ast_node* node = ast_node_new(alloc, kLiteralNode);
  node->as.literal.value = value;
  return node;
}

ast_node*
sequence_node_new(mun_alloc* alloc){
  ast_node* node = ast_node_new(alloc, kSequenceNode);
  node->as.sequence.children = malloc(sizeof(array));
  array_init(node->as.sequence.children, 0xA);
  return node;
}

ast_node*
binary_op_node_new(mun_alloc* alloc, ast_node* left, ast_node* right, binary_operation oper){
  ast_node* node = ast_node_new(alloc, kBinaryOpNode);
  node->as.binary_op.left = left;
  node->as.binary_op.right = right;
  node->as.binary_op.oper = oper;
  return node;
}

ast_node*
load_local_node_new(mun_alloc* alloc, local_variable* local){
  ast_node* node = ast_node_new(alloc, kLoadLocalNode);
  node->as.load_local.local = local;
  return node;
}

ast_node*
store_local_node_new(mun_alloc* alloc, local_variable* local, ast_node* value){
  ast_node* node = ast_node_new(alloc, kStoreLocalNode);
  node->as.store_local.local = local;
  node->as.store_local.value = value;
  return node;
}

ast_node*
load_indexed_node_new(mun_alloc* alloc, ast_node* table, ast_node* index){
  ast_node* node = ast_node_new(alloc, kLoadIndexedNode);
  node->as.load_indexed.index = index;
  node->as.load_indexed.table = table;
  return node;
}

ast_node*
store_indexed_node_new(mun_alloc* alloc, ast_node* table, ast_node* index, ast_node* value){
  ast_node* node = ast_node_new(alloc, kStoreLocalNode);
  node->as.store_indexed.index = index;
  node->as.store_indexed.table = table;
  node->as.store_indexed.value = value;
  return node;
}

ast_node*
native_call_node_new(mun_alloc* alloc, function* func){
  ast_node* node = ast_node_new(alloc, kNativeCallNode);
  node->as.native_call.func = func;
  return node;
}

ast_node*
static_call_node_new(mun_alloc* alloc, function* func){
  ast_node* node = ast_node_new(alloc, kStaticCallNode);
  node->as.static_call.func = func;
  return node;
}

void
ast_visit(ast_node_visitor* vis, ast_node* node){
  switch(node->type){
    case kLiteralNode: vis->visit_literal(vis, node); break;
    case kReturnNode: vis->visit_return(vis, node); break;
    case kSequenceNode: vis->visit_sequence(vis, node); break;
    case kBinaryOpNode: vis->visit_binary_op(vis, node); break;
    case kLoadLocalNode: vis->visit_load_local(vis, node); break;
    case kStoreLocalNode: vis->visit_store_local(vis, node); break;
    default:{
      fprintf(stderr, "Unhandled node type: %d\n", node->type);
      abort();
    }
  }
}