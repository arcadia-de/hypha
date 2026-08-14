#include "query_ast.h"

#include <stdlib.h>

static inline void FreeArgs(QueryArg* arg) {
  while (arg) {
    QueryArg* next = arg->next;
    free(arg->name);
    free(arg->value);
    free(arg);
    arg = next;
  }
}

static inline void FreeFields(AstField* field) {
  while (field) {
    AstField* next = field->next;
    free(field->name);
    FreeFields(field->sub_fields);
    free(field);

    field = next;
  }
}

void AstDocumentFree(AstDocument* doc) {
  AstSelection* sel = doc->selections;
  while (sel) {
    AstSelection* next = sel->next;
    free(sel->name);
    FreeArgs(sel->args);
    FreeFields(sel->fields);
    free(sel);

    sel = next;
  }

  doc->selections = NULL;
}
