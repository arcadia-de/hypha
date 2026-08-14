hypha_resource_graph_inc_dir = meson.current_source_dir() / 'include'
hypha_resource_graph_inc = include_directories('include/')
hypha_resource_graph = static_library(
  'hypha-resource-graph',
  [
    'src/resource.c',
    'src/resource_selector.c',
    'src/resource_graph.c',
    'src/resource_graph_gvc.c',
  ],
  dependencies: [
    libuv,
    threads,
    hypha_api_dep,
    hypha_log_dep,
  ],
  include_directories: [
    hypha_resource_graph_inc,
  ],
  install: true
)
hypha_resource_graph_dep = declare_dependency(
  link_with: [
    hypha_resource_graph,
  ],
  include_directories: [
    hypha_resource_graph_inc,
  ]
)

if gtest_dep.found()
  subdir('test/')
endif

