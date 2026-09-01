#include <stdlib.h>

#include "hypha.h"
#include "hypha/assertions.h"
#include "hypha/query.h"
#include "hypha/resource.h"
#include "hypha/resource_kind.h"
#include "hypha/resource_query_schema.h"
#include "hypha/resource_state.h"
#include "types.h"

static const size_t kInitSize = 8;

QuerySchema* NewHyphaResourcesQuerySchema() {
  QuerySchema* schema = (QuerySchema*)calloc(sizeof(QuerySchema), 1);
  schema->roots = (RootFieldDef*)calloc(sizeof(RootFieldDef), kInitSize);
  schema->roots_cap = kInitSize;
  schema->roots_len = 0;
  schema->types = (TypeDef*)calloc(sizeof(TypeDef), kInitSize);
  schema->types_len = 0;
  schema->types_cap = kInitSize;
  InitResourceAnnotationSchema(schema);
  InitResourceSchema(schema);
  InitManifestSchema(schema);
  return schema;
}
