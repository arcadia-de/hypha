#include "query_lexer.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void LexerInit(Lexer* lex, const char* src) {
  lex->src = src;
  lex->pos = 0;
  lex->line = 1;
  lex->col = 1;
}

static char Peek(Lexer* lex) {
  return lex->src[lex->pos];
}

static char Advance(Lexer* lex) {
  const char c = lex->src[lex->pos];
  if (c == '\0')
    return c;
  lex->pos++;
  if (c == '\n') {
    lex->line++;
    lex->col = 1;
  } else {
    lex->col++;
  }
  return c;
}

static void SkipWhitespaceAndComments(Lexer* lex) {
  for (;;) {
    const char c = Peek(lex);
    if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
      Advance(lex);
      continue;
    }

    if (c == '#') {
      while (Peek(lex) != '\n' && Peek(lex) != '\0')
        Advance(lex);
      continue;
    }

    break;
  }
}

static Token MakeToken(TokenKind kind, char* text, int line, int col) {
  Token tok = {.kind = kind, .text = text, .line = line, .col = col};
  return tok;
}

static Token ErrorToken(Lexer* lex, const char* msg) {
  char buf[256];
  snprintf(buf, sizeof(buf), "%s (line %d, col %d)", msg, lex->line, lex->col);
  return MakeToken(kInvalidToken, strdup(buf), lex->line, lex->col);
}

static Token LexString(Lexer* lex) {
  const int start_line = lex->line, start_col = lex->col;
  Advance(lex);

  int n = 0;
  char buf[4096];
  while (Peek(lex) != '"') {
    if (Peek(lex) == '\0')
      return ErrorToken(lex, "unterminated string literal");
    if (n >= (int)sizeof(buf) - 1)
      return ErrorToken(lex, "string literal too long");

    char c = Advance(lex);
    if (c == '\\') {
      const char esc = Advance(lex);
      switch (esc) {
        case '"':
          c = '"';
          break;
        case '\\':
          c = '\\';
          break;
        case 'n':
          c = '\n';
          break;
        case 't':
          c = '\t';
          break;
        default:
          return ErrorToken(lex, "unsupported escape sequence");
      }
    }

    buf[n++] = c;
  }
  Advance(lex);

  buf[n] = '\0';
  return MakeToken(kStringToken, strdup(buf), start_line, start_col);
}

static Token LexIdentOrKeyword(Lexer* lex) {
  const int start_line = lex->line, start_col = lex->col;
  char buf[256];
  int n = 0;

  while (isalnum((unsigned char)Peek(lex)) || Peek(lex) == '_') {
    if (n >= (int)sizeof(buf) - 1)
      return ErrorToken(lex, "identifier too long");

    buf[n++] = Advance(lex);
  }

  buf[n] = '\0';
  return MakeToken(kIdentifierToken, strdup(buf), start_line, start_col);
}

static Token LexNumber(Lexer* lex) {
  const int start_line = lex->line, start_col = lex->col;
  char buf[64];
  int n = 0;

  while (isdigit((unsigned char)Peek(lex)) || Peek(lex) == '.' || Peek(lex) == '-') {
    if (n >= (int)sizeof(buf) - 1)
      return ErrorToken(lex, "number literal too long");

    buf[n++] = Advance(lex);
  }

  buf[n] = '\0';
  return MakeToken(kNumberToken, strdup(buf), start_line, start_col);
}

Token LexerNext(Lexer* lex) {
  SkipWhitespaceAndComments(lex);

  const int line = lex->line, col = lex->col;
  const char c = Peek(lex);

  if (c == '\0')
    return MakeToken(kEofToken, NULL, line, col);

  if (c == '{') {
    Advance(lex);
    return MakeToken(kLBraceToken, NULL, line, col);
  }

  if (c == '}') {
    Advance(lex);
    return MakeToken(kRBraceToken, NULL, line, col);
  }

  if (c == '(') {
    Advance(lex);
    return MakeToken(kLParenToken, NULL, line, col);
  }

  if (c == ')') {
    Advance(lex);
    return MakeToken(kRParenToken, NULL, line, col);
  }

  if (c == ':') {
    Advance(lex);
    return MakeToken(kColonToken, NULL, line, col);
  }

  if (c == ',') {
    Advance(lex);
    return MakeToken(kCommaToken, NULL, line, col);
  }

  if (c == '"')
    return LexString(lex);

  if (isalpha((unsigned char)c) || c == '_')
    return LexIdentOrKeyword(lex);

  if (isdigit((unsigned char)c) || (c == '-' && isdigit((unsigned char)lex->src[lex->pos + 1])))
    return LexNumber(lex);

  return ErrorToken(lex, "unexpected character");
}

void TokenFree(Token* tok) {
  free(tok->text);
  tok->text = NULL;
}
