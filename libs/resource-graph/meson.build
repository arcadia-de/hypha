hypha_resource_graph_inc = include_directories('include/')
hypha_resource_graph_deps = [
  libuv,
  threads,
  uuid,
  jansson,
  hypha_api_dep,
  hypha_log_dep,
  hypha_utils_dep,
]
hypha_resource_graph = static_library(
  'hypha-resource-graph',
  [
    'src/resource.c',
    'src/resource_bootstrap.c',
    'src/resource_decorator.c',
    'src/resource_selector.c',
    'src/resource_graph.c',
    'src/resource_kind.c',
    'src/resource_graph_gvc.c',
    'src/resource_scheduler_dfs.c',
    'src/resource_scheduler_kahn.c',
    'src/resource_validator.c',
  ],
  dependencies: hypha_resource_graph_deps,
  include_directories: [
    hypha_resource_graph_inc,
  ],
  install: false,
)
hypha_resource_graph_dep = declare_dependency(
  link_with: hypha_resource_graph,
  dependencies: hypha_resource_graph_deps,
  include_directories: [
    hypha_resource_graph_inc,
  ],
)

if gtest_dep.found()
  subdir('test/')
endif

