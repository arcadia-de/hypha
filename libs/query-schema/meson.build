query_schema_inc = include_directories('include/')
query_schema_deps = [
  libuv,
  threads,
  hypha_query_dep,
  hypha_resource_graph_dep,
]
query_schema = static_library(
  'hypha-query-schema',
  [
    'src/types.c',
    'src/type_manifest.c',
    'src/type_resource.c',
    'src/type_resource_annotation.c',
    'src/query_schema_resources.c',
  ],
  dependencies: query_schema_deps,
  include_directories: [
    query_schema_inc,
  ],
  install: false,
)
query_schema_dep = declare_dependency(
  link_with: query_schema,
  dependencies: query_schema_deps,
  include_directories: [
    query_schema_inc,
  ],
)
