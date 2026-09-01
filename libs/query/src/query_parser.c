#include "query_parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "query_lexer.h"

typedef struct {
  Lexer lex;
  Token cur;
  char* error;
} ParserState;

static void Advance(ParserState* p) {
  TokenFree(&p->cur);
  p->cur = LexerNext(&p->lex);
}

static bool Fail(ParserState* p, const char* msg) {
  if (!p->error) {
    char buf[300];
    snprintf(buf, sizeof(buf), "%s (line %d, col %d)", msg, p->cur.line, p->cur.col);
    p->error = strdup(buf);
  }

  return false;
}

static bool Expect(ParserState* p, TokenKind kind, const char* what) {
  if (p->cur.kind == kInvalidToken)
    return Fail(p, p->cur.text ? p->cur.text : "lexical error");

  if (p->cur.kind != kind) {
    char buf[128];
    snprintf(buf, sizeof(buf), "expected %s", what);
    return Fail(p, buf);
  }

  return true;
}

static QueryArg* ParseArgList(ParserState* p) {
  Advance(p);

  QueryArg* head = NULL;
  QueryArg* tail = NULL;

  for (;;) {
    if (!Expect(p, kIdentifierToken, "an argument name"))
      return head;

    QueryArg* arg = (QueryArg*)calloc(1, sizeof(QueryArg));
    arg->name = strdup(p->cur.text);
    Advance(p);

    if (!Expect(p, kColonToken, "':' after argument name")) {
      free(arg->name);
      free(arg);
      return head;
    }
    Advance(p);

    if (p->cur.kind != kIdentifierToken && p->cur.kind != kStringToken && p->cur.kind != kNumberToken) {
      Fail(p, "expected an argument value");
      free(arg->name);
      free(arg);
      return head;
    }
    arg->value = strdup(p->cur.text);
    Advance(p);

    if (tail) {
      tail->next = arg;
    } else {
      head = arg;
    }
    tail = arg;

    if (p->cur.kind == kCommaToken) {
      Advance(p);
      continue;
    }

    break;
  }

  if (!Expect(p, kRParenToken, "')' to close argument list"))
    return head;
  Advance(p);

  return head;
}

static AstField* ParseFieldList(ParserState* p) {
  Advance(p);

  AstField* head = NULL;
  AstField* tail = NULL;

  while (p->cur.kind == kIdentifierToken) {
    AstField* field = (AstField*)calloc(1, sizeof(AstField));
    field->name = strdup(p->cur.text);
    Advance(p);

    if (p->cur.kind == kLBraceToken)
      field->sub_fields = ParseFieldList(p);

    if (tail) {
      tail->next = field;
    } else {
      head = field;
    }
    tail = field;

    if (p->error)
      return head;
  }

  if (!Expect(p, kRBraceToken, "'}' to close selection"))
    return head;
  Advance(p);

  return head;
}

static AstSelection* ParseSelection(ParserState* p) {
  AstSelection* sel = (AstSelection*)calloc(1, sizeof(AstSelection));
  sel->name = strdup(p->cur.text);
  Advance(p);

  if (p->cur.kind == kLParenToken)
    sel->args = ParseArgList(p);

  if (p->error)
    return sel;

  if (!Expect(p, kLBraceToken, "'{' to begin a field selection"))
    return sel;

  sel->fields = ParseFieldList(p);
  return sel;
}

bool ParseQuery(const char* query_text, AstDocument* out_doc, char** out_error) {
  ParserState p = {.error = NULL};
  LexerInit(&p.lex, query_text);
  p.cur = LexerNext(&p.lex);

  out_doc->selections = NULL;
  AstSelection* tail = NULL;

  if (p.cur.kind == kEofToken) {
    TokenFree(&p.cur);
    *out_error = strdup("empty query");
    return false;
  }

  while (p.cur.kind == kIdentifierToken && !p.error) {
    AstSelection* sel = ParseSelection(&p);

    if (tail) {
      tail->next = sel;
    } else {
      out_doc->selections = sel;
      out_doc->num_selections++;
    }

    tail = sel;
  }

  if (!p.error && p.cur.kind != kEofToken)
    Fail(&p, "expected another selection or end of query");

  TokenFree(&p.cur);

  if (p.error) {
    AstDocumentFree(out_doc);
    *out_error = p.error;
    return false;
  }

  *out_error = NULL;
  return true;
}
