hypha_state_inc = include_directories('include/')
hypha_state_deps = [
  libuv,
  threads,
  hypha_api_dep,
  hypha_log_dep,
  hypha_utils_dep,
]
hypha_state = static_library(
  'hypha-state',
  [
    'src/state_store.c',
    'src/state_compactor.c',
    'src/state_index.c',
    'src/state_log.c',
  ],
  dependencies: hypha_state_deps,
  include_directories: hypha_state_inc,
  install: false,
)
hypha_state_dep = declare_dependency(
  link_with: hypha_state,
  dependencies: hypha_state_deps,
  include_directories: hypha_state_inc,
)
