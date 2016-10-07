#ifndef MUN_PARSER_H
#define MUN_PARSER_H

#include "common.h"

HEADER_BEGIN

#include "scope.h"

#define FOR_EACH_TOKEN_KW(V) \
  V(kAND, "and") \
  V(kBREAK, "break") \
  V(kDO, "do") \
  V(kELSE, "else") \
  V(kELSEIF, "elseif") \
  V(kEND, "end") \
  V(kFALSE, "false") \
  V(kFOR, "for") \
  V(kFUNCTION, "function") \
  V(kIF, "if") \
  V(kIN, "in") \
  V(kLOCAL, "local") \
  V(kNIL, "nil") \
  V(kNOT, "not") \
  V(kOR, "or") \
  V(kREPEAT, "repeat") \
  V(kRETURN, "return") \
  V(kTHEN, "then") \
  V(kTRUE, "true") \
  V(kUNTIL, "until") \
  V(kWHILE, "while")

#define FOR_EACH_TOKEN_SYM(V) \
  V(kEQUALS, "=") \
  V(kLPAREN, "(") \
  V(kRPAREN, ")") \
  V(kLBRACKET, "[") \
  V(kRBRACKET, "]") \
  V(kLBRACE, "{") \
  V(kRBRACE, "}") \
  V(kCOMMA, ",") \
  V(kADD, "+") \
  V(kSUB, "-") \
  V(kMUL, "*") \
  V(kDIV, "/") \
  V(kSEMICOLON, ";")

#define FOR_EACH_TOKEN_LIT(V) \
  V(kLIT_STRING, "<literal string>") \
  V(kLIT_NUMBER, "<literal number>")

#define FOR_EACH_TOKEN(V) \
  FOR_EACH_TOKEN_LIT(V) \
  FOR_EACH_TOKEN_SYM(V) \
  FOR_EACH_TOKEN_KW(V)

typedef enum{
#define DEFINE_TYPE(Token, NameDesc) Token,
  FOR_EACH_TOKEN(DEFINE_TYPE)
#undef DEFINE_TYPE
  kIDENTIFIER,
  kEOF,
} token_type;

typedef struct{
  token_type type;
  char* text;
} token;

MUN_INLINE token*
token_new(token_type type, char* text){
  token* tok = malloc(sizeof(token));
  tok->text = strdup(text);
  tok->type = type;
  return tok;
}

typedef struct{
  FILE* file;
  token* peek;
  local_scope* scope;
  mun_alloc* gc;
} parser;

parser* parser_new(mun_alloc* alloc);

void parse(parser* parse, instance* script);

HEADER_END

#endif