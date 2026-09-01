#include "hypha/annotation.h"
#include "types.h"

static FieldResolverResult AnnotationFieldName(void* obj) {
  return (FieldResolverResult){.kind = kQueryFieldResultScalar, .scalar = strdup(((Annotation*)obj)->key)};
}

static FieldResolverResult AnnotationFieldValue(void* obj) {
  return (FieldResolverResult){.kind = kQueryFieldResultScalar, .scalar = strdup(((Annotation*)obj)->value)};
}

static const FieldDef kResourceAnnotationFields[2] = {
    {"name", AnnotationFieldName},
    {"value", AnnotationFieldValue},
};

bool InitResourceAnnotationSchema(QuerySchema* schema) {
  ASSERT(schema);
  return true;
}
