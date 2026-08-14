runner_exec = executable(
  'hypha-test-runner',
  [
    'main.c',
  ],
  dependencies: all_hypha_deps + [
    hypha_api_dep,
    hypha_dep,
    hypha_events_dep,
    hypha_resource_graph_dep,
  ],
  include_directories: [
    hypha_inc,
  ],
  install: false,
)
