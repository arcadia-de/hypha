#ifndef HYPHA_QUERY_PARSER_H
#define HYPHA_QUERY_PARSER_H

#include <stdbool.h>

#include "query_ast.h"

bool ParseQuery(const char* query_text, AstDocument* out_doc, char** out_error);

#endif  // HYPHA_QUERY_PARSER_H
