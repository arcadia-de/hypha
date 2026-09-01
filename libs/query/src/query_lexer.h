#ifndef HYPHA_QUERY_LEXER_H
#define HYPHA_QUERY_LEXER_H

#define FOR_EACH_QUERY_TOKEN(V) \
  V(Identifier)                 \
  V(String)                     \
  V(Number)                     \
  V(LBrace)                     \
  V(RBrace)                     \
  V(LParen)                     \
  V(RParen)                     \
  V(Colon)                      \
  V(Comma)                      \
  V(Eof)

// clang-format off
typedef enum {
  kInvalidToken,
#define DEFINE_KIND(Name) \
  k##Name##Token,
  FOR_EACH_QUERY_TOKEN(DEFINE_KIND)
#undef DEFINE_KIND
  kTotalNumberOfTokens,
} TokenKind;
// clang-format on

typedef struct {
  TokenKind kind;
  char* text;
  int line;
  int col;
} Token;

typedef struct {
  const char* src;
  int pos;
  int line;
  int col;
} Lexer;

void LexerInit(Lexer* lex, const char* src);
Token LexerNext(Lexer* lex);
void TokenFree(Token* tok);

#endif  // HYPHA_QUERY_LEXER_H
