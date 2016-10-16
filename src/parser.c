#include <mun/parser.h>
#include <mun/ast.h>
#include <mun/alloc.h>
#include <ctype.h>
#include <mun/function.h>
#include <mun/type.h>
#include <mun/array.h>

static int
lex_keyword(char* kw){
#define LEX_KEYWORD(Token, NameDesc) \
  if(strcmp(kw, NameDesc) == 0) return Token;
FOR_EACH_TOKEN_KW(LEX_KEYWORD);
#undef LEX_KEYWORD
  return -1;
}

MUN_INLINE char
parser_peek(parser* parse){
  int c = getc(parse->file);
  ungetc(c, parse->file);
  return ((char) c);
}

MUN_INLINE char
parser_next(parser* parse){
  int c = getc(parse->file);
  return ((char) c);
}

MUN_INLINE char
parser_next_real(parser* parse){
  char next;
  while(isspace(next = parser_next(parse)));
  return next;
}

MUN_INLINE bool
issymbol(char c){
  return c == '(' ||
         c == ')' ||
         c == '[' ||
         c == ']' ||
         c == '{' ||
         c == '}' ||
         c == '=' ||
         c == ':' ||
         c == ';' ||
         c == ',' ||
         c == '*' ||
         c == '/' ||
         c == '+' ||
         c == '-' ||
         c == '.' ||
         c == ',' ||
         c == '?' ||
         c == '<' ||
         c == '>';
}

MUN_INLINE bool
iskeyword(char* str){
  return lex_keyword(str) != -1;
}

token*
next_token(parser* parse){
  if(parse->peek != NULL){
    token* next = parse->peek;
    parse->peek = NULL;
    return next;
  }

  char next = parser_next_real(parse);
  switch(next){
    case EOF: return token_new(kEOF, "");
    case ',': return token_new(kCOMMA, ",");
    case '=': return token_new(kEQUALS, "=");
    case '+': return token_new(kADD, "+");
    case '*': return token_new(kMUL, "*");
    case '-': return token_new(kSUB, "-");
    case '/': return token_new(kDIV, "/");
    default: break;
  }

  if(next == '"'){
    sstream buffer;
    sstream_init(&buffer);
    while((next = parser_next(parse)) != '"') sstream_putc(&buffer, next);
    token* result = token_new(kLIT_STRING, sstream_cstr(&buffer));
    sstream_dispose(&buffer);
    return result;
  } else if(isdigit(next)){
    sstream buffer;
    sstream_init(&buffer);
    sstream_putc(&buffer, next);
    bool dbl = FALSE;
    while(isdigit(next = parser_peek(parse)) || (next == '.' && !dbl)){
      sstream_putc(&buffer, next);
      if(next == '.') dbl = TRUE;
      parser_next(parse);
    }
    token* result = token_new(kLIT_NUMBER, sstream_cstr(&buffer));
    sstream_dispose(&buffer);
    return result;
  } else{
    sstream buffer;
    sstream_init(&buffer);
    sstream_putc(&buffer, next);
    while(!isspace(next = parser_peek(parse)) && !issymbol(next)){
      sstream_putc(&buffer, next);
      parser_next(parse);

      char* data = sstream_cstr(&buffer);
      if(iskeyword(data)){
        token* result = token_new(((token_type) lex_keyword(data)), data);
        sstream_dispose(&buffer);
        return result;
      }
    }

    token* result = token_new(kIDENTIFIER, sstream_cstr(&buffer));
    sstream_dispose(&buffer);
    return result;
  }
}

#define CONSUME \
  next = next_token(parse)

#define UNEXPECTED \
  fprintf(stderr, "Unexpected %s$%d\n", next->text, next->type); \
  getchar(); \
  abort();

#define DECONSUME \
  parse->peek = next;

#define ALLOC \
  parse->gc

static ast_node*
parse_unary_expr(parser* parse){
  ast_node* primary = NULL;

  token* next;
  switch((CONSUME)->type){
    case kLIT_STRING:{
      primary = literal_node_new(parse->gc, string_new(parse->gc, next->text));
      break;
    }
    case kLIT_NUMBER:{
      primary = literal_node_new(parse->gc, number_new(parse->gc, atof(next->text)));
      break;
    }
    default:{
      UNEXPECTED
    }
  }

  return primary;
}

MUN_INLINE bool
isexpr(token* tok){
  switch(tok->type){
    case kADD:
    case kSUB:
    case kMUL:
    case kDIV:
      return TRUE;
    default:
      return FALSE;
  }
}

MUN_INLINE binary_operation
to_binary_op(token_type type){
  switch(type){
    case kADD: return kAdd;
    case kSUB: return kSubtract;
    case kMUL: return kMultiply;
    case kDIV: return kDivide;
    default:{
      fprintf(stderr, "Unknown binary operation: %d\n", type);
      abort();
    }
  }
}

static ast_node*
parse_binary_expr(parser* parse){
  ast_node* left = parse_unary_expr(parse);

  token* next;
  while(isexpr(CONSUME)){
    left = binary_op_node_new(ALLOC, left, parse_binary_expr(parse), to_binary_op(next->type));
  }

  DECONSUME;
  return left;
}

static void
parse_statement(parser* parse, instance* script){
  token* next;
  switch((CONSUME)->type){
    case kSEMICOLON: break;
    case kLOCAL:{
      if((CONSUME)->type == kIDENTIFIER){
        array varlist; // char*
        ARRAY(varlist);
        array_add(&varlist, next->text);

        if((CONSUME)->type == kCOMMA) {
          do {
            array_add(&varlist, (CONSUME)->text);
          } while ((CONSUME)->type == kCOMMA);
        }

        if(next->type != kEQUALS){
          UNEXPECTED;
        }

        for(int i = 0; i < varlist.size; i++){
          local_variable* local = local_var_new(varlist.data[i]);
          ast_node* init = parse_binary_expr(parse);
          if(init->type == kLiteralNode){
            local->value = init->as.literal.value;
          }
          if(!local_scope_add(parse->scope, local)){
            fprintf(stderr, "Duplicate local '%s' in scope\n", ((char*) varlist.data[i]));
            abort();
          }

          sequence_node_append(script->as.script.main->ast, store_local_node_new(ALLOC, local, init));
          if(i < (varlist.size - 1) && (CONSUME)->type != kCOMMA){
            UNEXPECTED;
          }
        }
      }
      break;
    }
    default:{
      UNEXPECTED;
    }
  }
}

void
parse(parser* parse, instance* script){
  parse->file = fopen(script->as.script.url, "r");

  while(parser_peek(parse) != EOF){
    parse_statement(parse, script);
  }

  ast_node* seq = script->as.script.main->ast;
  if(((ast_node*) array_last(seq->as.sequence.children))->type != kReturnNode){
    ast_node* prev = array_last(seq->as.sequence.children);
    switch(prev->type){
      case kStoreLocalNode: prev = prev->as.store_local.value; break;
      default: break;
    }
    array_insert(seq->as.sequence.children, seq->as.sequence.children->size - 1, return_node_new(parse->gc, prev));
  }

  fclose(parse->file);
}

parser*
parser_new(mun_alloc* alloc){
  parser* parse = mun_gc_alloc(alloc, sizeof(parser));
  parse->file = NULL;
  parse->gc = alloc;
  parse->peek = NULL;
  parse->scope = local_scope_new(NULL);
  return parse;
}