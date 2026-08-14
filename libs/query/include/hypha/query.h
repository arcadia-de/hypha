#ifndef HYPHA_QUERY_H
#define HYPHA_QUERY_H

#include <stdint.h>

typedef struct _QueryArg {
  char* name;
  char* value;
  struct _QueryArg* next;
} QueryArg;

const char* QueryArgGet(const QueryArg* args, const char* name);

typedef struct {
  void* object;
  const char* type_name;
} QueryObject;

#define FOR_EACH_QUERY_FIELD_RESULT_KIND(V) \
  V(Scalar)                                 \
  V(ScalarList)                             \
  V(ObjectList)

// clang-format off
typedef enum {
#define DEFINE_KIND(Name) kQueryFieldResult##Name,
  FOR_EACH_QUERY_FIELD_RESULT_KIND(DEFINE_KIND)
#undef DEFINE_KIND
  kTotalNumberOfQueryFieldResultKinds,
} FieldResultKind;
// clang-format on

typedef struct {
  FieldResultKind kind;

  char* scalar;

  char** scalar_list;
  uint32_t scalar_list_count;

  QueryObject* object_list;
  uint32_t object_list_count;
} FieldResolverResult;

typedef FieldResolverResult (*FieldResolverFn)(void* object);

typedef struct {
  const char* name;
  FieldResolverFn resolve;
} FieldDef;

typedef struct {
  const char* type_name;
  const FieldDef* fields;
  uint32_t num_fields;
} TypeDef;

typedef struct {
  QueryObject* objects;
  uint32_t count;
} RootResult;

typedef RootResult (*RootResolverFn)(const QueryArg* args, void* context);

typedef struct {
  const char* name;
  RootResolverFn resolve;
  const char* result_type;
} RootFieldDef;

typedef struct {
  const RootFieldDef* roots;
  uint32_t num_roots;
  const TypeDef* types;
  uint32_t num_types;
  void* context;
} QuerySchema;

#define FOR_EACH_QUERY_RESULT_KIND(V) \
  V(Null)                             \
  V(String)                           \
  V(Object)                           \
  V(Array)

// clang-format off
typedef enum {
#define DEFINE_KIND(Name) kQueryResult##Name,
  FOR_EACH_QUERY_RESULT_KIND(DEFINE_KIND)
#undef DEFINE_KIND
  kTotalNumberOfQueryResultKinds
} ResultKind;
// clang-format on

typedef struct _QueryResult QueryResult;

typedef struct {
  char* key;
  QueryResult* value;
} ResultObjectField;

struct _QueryResult {
  ResultKind kind;

  char* string_value;

  ResultObjectField* object_fields;
  uint32_t num_object_fields;

  QueryResult** array_items;
  uint32_t num_array_items;
};

void ResultNodeFree(QueryResult* node);
char* ResultNodeToJSON(const QueryResult* node);
QueryResult* QueryExecute(const QuerySchema* schema, const char* query_text, char** out_error);

#endif  // HYPHA_QUERY_H
