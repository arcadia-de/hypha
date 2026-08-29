hypha_action_inc = include_directories('include/')
hypha_action_deps = [
  libuv,
  threads,
  hypha_api_dep,
  hypha_log_dep,
  hypha_utils_dep,
  hypha_resource_graph_dep,
]
hypha_action = static_library(
  'hypha-action',
  [
    'src/action_log.c',
  ],
  dependencies: hypha_action_deps,
  include_directories: [
    hypha_action_inc,
  ],
  install: false,
)
hypha_action_dep = declare_dependency(
  link_with: hypha_action,
  dependencies: hypha_action_deps,
  include_directories: [
    hypha_action_inc,
  ],
)

# if gtest_dep.found()
#   subdir('test/')
# endif
