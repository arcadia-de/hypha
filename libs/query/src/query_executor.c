#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hypha/assertions.h"
#include "hypha/query.h"
#include "query_ast.h"
#include "query_parser.h"

const char* QueryArgGet(const QueryArg* args, const char* name) {
  for (const QueryArg* a = args; a; a = a->next) {
    if (strcmp(a->name, name) == 0)
      return a->value;
  }

  return NULL;
}

static const RootFieldDef* FindRoot(const QuerySchema* schema, const char* name) {
  for (uint32_t i = 0; i < schema->roots_len; i++) {
    if (strcmp(schema->roots[i].name, name) == 0)
      return &schema->roots[i];
  }

  return NULL;
}

static const TypeDef* FindType(const QuerySchema* schema, const char* type_name) {
  for (uint32_t i = 0; i < schema->types_len; i++) {
    if (strcmp(schema->types[i].type_name, type_name) == 0)
      return &schema->types[i];
  }

  return NULL;
}

static const FieldDef* FindField(const TypeDef* type, const char* name) {
  for (uint32_t i = 0; i < type->num_fields; i++) {
    if (strcmp(type->fields[i].name, name) == 0)
      return &type->fields[i];
  }

  return NULL;
}

static inline QueryResult* NewNode(ResultKind kind) {
  QueryResult* node = (QueryResult*)calloc(1, sizeof(QueryResult));
  node->kind = kind;
  return node;
}

static inline QueryResult* NewErrorNode(char* message, const size_t message_len) {
  ASSERT(message);
  ASSERT(message_len > 0);
  QueryResult* node = (QueryResult*)malloc(sizeof(QueryResult));
  if (node) {
    node->kind = kQueryResultError;
    node->message = strndup(message, message_len);
    node->message_len = message_len;
  }
  return node;
}

static void FreeFieldResolverResult(FieldResolverResult* r) {
  switch (r->kind) {
    case kQueryFieldResultScalar:
      free(r->scalar);
      break;
    case kQueryFieldResultScalarList:
      for (uint32_t i = 0; i < r->scalar_list_count; i++)
        free(r->scalar_list[i]);
      free(r->scalar_list);
      break;
    case kQueryFieldResultObjectList:
      free(r->object_list);
      break;
    default:
      break;
  }
}

static QueryResult* ResolveObject(const QuerySchema* schema, const TypeDef* type, void* object, const AstField* fields,
                                  char** out_error);

static QueryResult* ResolveField(const QuerySchema* schema, const TypeDef* type, void* object, const AstField* field,
                                 char** out_error) {
  const FieldDef* def = FindField(type, field->name);
  if (!def) {
    char buf[160];
    snprintf(buf, sizeof(buf), "unknown field '%s' on type '%s'", field->name, type->type_name);
    *out_error = strdup(buf);
    return NULL;
  }

  FieldResolverResult r = def->resolve(object);
  QueryResult* result = NULL;

  switch (r.kind) {
    case kQueryFieldResultScalar:
      if (r.scalar) {
        result = NewNode(kQueryResultString);
        result->string_value = r.scalar;
        r.scalar = NULL;
      } else {
        result = NewNode(kQueryResultNull);
      }

      break;
    case kQueryFieldResultScalarList: {
      result = NewNode(kQueryResultArray);
      result->num_array_items = r.scalar_list_count;
      result->array_items = (QueryResult**)malloc(sizeof(QueryResult*) * r.scalar_list_count);
      for (uint32_t i = 0; i < r.scalar_list_count; i++) {
        QueryResult* item = NewNode(kQueryResultString);
        item->string_value = r.scalar_list[i];
        r.scalar_list[i] = NULL;
        result->array_items[i] = item;
      }

      break;
    }

    case kQueryFieldResultObjectList: {
      if (!field->sub_fields) {
        char buf[160];
        snprintf(buf, sizeof(buf), "field '%s' returns objects and requires a sub-selection, e.g. '%s { ... }'",
                 field->name, field->name);
        *out_error = strdup(buf);
        FreeFieldResolverResult(&r);
        return NULL;
      }

      result = NewNode(kQueryResultArray);
      result->num_array_items = r.object_list_count;
      result->array_items = (QueryResult**)malloc(sizeof(QueryResult*) * r.object_list_count);

      for (uint32_t i = 0; i < r.object_list_count; i++) {
        const TypeDef* item_type = FindType(schema, r.object_list[i].type_name);
        if (!item_type) {
          char buf[160];
          snprintf(buf, sizeof(buf), "internal error: unknown type '%s' returned by field '%s'",
                   r.object_list[i].type_name, field->name);
          *out_error = strdup(buf);
          FreeFieldResolverResult(&r);
          ResultNodeFree(result);
          return NULL;
        }

        result->array_items[i] =
            ResolveObject(schema, item_type, r.object_list[i].object, field->sub_fields, out_error);
        if (*out_error) {
          FreeFieldResolverResult(&r);
          ResultNodeFree(result);
          return NULL;
        }
      }

      break;
    }
    default:
      break;
  }

  FreeFieldResolverResult(&r);
  return result;
}

static QueryResult* ResolveObject(const QuerySchema* schema, const TypeDef* type, void* object, const AstField* fields,
                                  char** out_error) {
  QueryResult* obj_node = NewNode(kQueryResultObject);

  uint32_t count = 0;
  for (const AstField* f = fields; f; f = f->next)
    count++;

  obj_node->object_fields = (ResultObjectField*)malloc(sizeof(ResultObjectField) * count);
  obj_node->num_object_fields = count;

  uint32_t i = 0;
  for (const AstField* f = fields; f; f = f->next, i++) {
    obj_node->object_fields[i].key = strdup(f->name);
    obj_node->object_fields[i].value = ResolveField(schema, type, object, f, out_error);

    if (*out_error) {
      obj_node->num_object_fields = i + 1;
      ResultNodeFree(obj_node);
      return NULL;
    }
  }

  return obj_node;
}

static QueryResult* ExecuteSelection(const QuerySchema* schema, void* ctx, const AstSelection* sel, char** out_error) {
  const RootFieldDef* root = FindRoot(schema, sel->name);
  if (!root) {
    char buf[160];
    snprintf(buf, sizeof(buf), "unknown root field '%s'", sel->name);
    *out_error = strdup(buf);
    return NULL;
  }

  const TypeDef* type = FindType(schema, root->result_type);
  if (!type) {
    char buf[160];
    snprintf(buf, sizeof(buf), "internal error: unknown type '%s' for root field '%s'", root->result_type, sel->name);
    *out_error = strdup(buf);
    return NULL;
  }

  RootResult rr = root->resolve(sel->args, ctx);

  QueryResult* array = NewNode(kQueryResultArray);
  array->num_array_items = rr.count;
  array->array_items = (QueryResult**)malloc(sizeof(QueryResult*) * (rr.count ? rr.count : 1));

  for (uint32_t i = 0; i < rr.count; i++) {
    array->array_items[i] = ResolveObject(schema, type, rr.objects[i].object, sel->fields, out_error);
    if (*out_error) {
      array->num_array_items = i;
      ResultNodeFree(array);
      free(rr.objects);
      return NULL;
    }
  }

  free(rr.objects);
  return array;
}

QueryResult* QueryExecute(const QuerySchema* schema, void* ctx, const char* query_text) {
  AstDocument doc;
  memset(&doc, 0, sizeof(AstDocument));

  QueryResult* result = NULL;
  char* out_error = NULL;
  if (!ParseQuery(query_text, &doc, &out_error))
    goto failed;

  result = NewNode(kQueryResultObject);
  const size_t num_selections = doc.num_selections;

  result->object_fields = (ResultObjectField*)malloc(sizeof(ResultObjectField) * num_selections);
  result->num_object_fields = num_selections;

  uint32_t i = 0;
  for (AstSelection* s = doc.selections; s; s = s->next, i++) {
    result->object_fields[i].key = strdup(s->name);
    result->object_fields[i].value = ExecuteSelection(schema, ctx, s, &out_error);
    if (out_error)
      goto failed;
  }

  goto finished;

failed:
  if (result) {
    for (size_t j = 0; j < i; i++) {
      free(result->object_fields[j].key);
      free(result->object_fields[j].value);
    }

    free(result->object_fields);
    free(result);
  }

  result = NewErrorNode(out_error, strlen(out_error));

finished:
  AstDocumentFree(&doc);
  return result;
}
