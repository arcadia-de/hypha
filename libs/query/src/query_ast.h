#ifndef HYPHA_QUERY_AST_H
#define HYPHA_QUERY_AST_H

#include "hypha/query.h"

typedef struct _AstField {
  struct _AstField* next;

  char* name;
  struct _AstField* sub_fields;
} AstField;

typedef struct _AstSelection {
  struct _AstSelection* next;

  char* name;
  QueryArg* args;
  AstField* fields;
} AstSelection;

typedef struct {
  AstSelection* selections;
  size_t num_selections;
} AstDocument;

void AstDocumentFree(AstDocument* doc);

#endif  // HYPHA_QUERY_AST_H
