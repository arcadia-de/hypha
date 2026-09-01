#ifndef HYPHA_QUERY_TYPES_H
#define HYPHA_QUERY_TYPES_H

#include <stdlib.h>

#include "hypha/assertions.h"
#include "hypha/query.h"

void AppendRoots(QuerySchema* schema, RootFieldDef* defs, const size_t num_defs);

static inline void AppendRoot(QuerySchema* schema, RootFieldDef* def) {
  if (!schema || !def)
    return;
  return AppendRoots(schema, def, 1);
}

void AppendTypes(QuerySchema* schema, TypeDef* defs, const size_t num_defs);

static inline void AppendType(QuerySchema* schema, TypeDef* def) {
  if (!schema || !def)
    return;
  return AppendTypes(schema, def, 1);
}

#define DECLARE_ROOT_TYPE(Name) bool Init##Name##Schema(QuerySchema* schema);

DECLARE_ROOT_TYPE(ResourceAnnotation);
DECLARE_ROOT_TYPE(Manifest);
DECLARE_ROOT_TYPE(Resource);

#define _BIND_ROOT_FIELD(Name) {#Name, Resolve##Name##Field},

#define DEFINE_ROOT_TYPE(Name, Collection, ForEachField, NumberOfFields)                    \
  static const size_t kTotalNumberOf##Name##Fields = NumberOfFields;                        \
  static inline RootResult Resolve##Name(const QueryArg* args, void* ptr);                  \
  static const FieldDef k##Name##Fields[NumberOfFields] = {ForEachField(_BIND_ROOT_FIELD)}; \
  static const RootFieldDef k##Name##Root = {#Collection, &Resolve##Name, #Name};           \
  static const TypeDef k##Name##Type = {#Name, k##Name##Fields, NumberOfFields};            \
  bool Init##Name##Schema(QuerySchema* schema) {                                            \
    AppendType(schema, (TypeDef*)&k##Name##Type);                                           \
    AppendRoot(schema, (RootFieldDef*)&k##Name##Root);                                      \
    return true;                                                                            \
  }                                                                                         \
  RootResult Resolve##Name(const QueryArg* args, void* data)
// clang-format on

#define FIELD(Key, Name) {#Key, ResourceField##Name}

#define _FIELD_RESOLVER_FN(Type, TypeName, Name)                           \
  static inline FieldResolverResult DoResolve##Name##Field(TypeName* ptr); \
  static inline FieldResolverResult Resolve##Name##Field(void* ptr) {      \
    return DoResolve##Name##Field((TypeName*)ptr);                         \
  }                                                                        \
  static inline FieldResolverResult DoResolve##Name##Field(TypeName* ptr)

#endif  // HYPHA_QUERY_TYPES_H
