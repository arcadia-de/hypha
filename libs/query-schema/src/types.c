#include "types.h"

#include <string.h>

#include "hypha/assertions.h"
#include "hypha/query.h"

static inline void EnsureRootsCap(QuerySchema* schema, const size_t new_len) {
  ASSERT(schema);
  if (new_len < schema->roots_cap)
    return;

  const size_t new_cap = schema->roots_cap + new_len;
  const size_t total_size = sizeof(RootFieldDef) * new_cap;
  RootFieldDef* new_roots = (RootFieldDef*)realloc(schema->roots, total_size);
  ASSERT(new_roots);
  schema->roots = new_roots;
  schema->roots_cap = new_cap;
}

void AppendRoots(QuerySchema* schema, RootFieldDef* defs, const size_t num_defs) {
  if (!schema || !defs || num_defs == 0)
    return;

  EnsureRootsCap(schema, schema->roots_len + num_defs);
  const size_t total_size = sizeof(RootFieldDef) * num_defs;
  memcpy(&schema->roots[schema->roots_len], defs, total_size);
  schema->roots_len += num_defs;
}

static inline void EnsureTypesCap(QuerySchema* schema, const size_t new_len) {
  ASSERT(schema);
  if (new_len < schema->types_cap)
    return;

  const size_t new_cap = schema->types_cap + new_len;
  const size_t total_size = sizeof(TypeDef) * new_cap;
  TypeDef* new_types = (TypeDef*)realloc(schema->types, total_size);
  ASSERT(new_types);
  schema->types = new_types;
  schema->types_cap = new_cap;
}

void AppendTypes(QuerySchema* schema, TypeDef* defs, const size_t num_defs) {
  if (!schema || !defs || num_defs == 0)
    return;

  EnsureTypesCap(schema, schema->types_len + num_defs);
  const size_t total_size = sizeof(TypeDef) * num_defs;
  memcpy(&schema->types[schema->types_len], defs, total_size);
  schema->types_len += num_defs;
}
