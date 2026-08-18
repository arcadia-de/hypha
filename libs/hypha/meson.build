hypha_inc_dir = meson.current_source_dir() / 'include'
hypha_inc = include_directories('include/')
hypha_deps = [
  libuv,
  threads,
  jansson,
  lua54,
  wamr,
  xxhash,
  gvc,
  hypha_api_dep,
  pkg_manager_api_dep,
  hypha_query_dep,
  hypha_events_dep,
  hypha_state_dep,
  hypha_utils_dep,
  hypha_controller_dep,
  hypha_resource_graph_dep,
]
hypha_deps += pkg_manager_deps
hypha_deps += controllers_deps
hypha = static_library(
  'hypha',
  [
    'src/hypha.c',
    'src/decorator.c',
    'src/orchestrator.c',
    'src/orchestrator_gvc.c',
    'src/orchestrator_lua.c',
    'src/reconcile.c',
    'src/discovery.c',
    'src/history_log.c',
    'src/query_schema_resources.c',
    'src/package_managers.c',
    'src/planner.c',
    'src/planned_action.c',
  ],
  dependencies: hypha_deps,
  link_with: [
    hypha_log,
    hypha_utils,
  ],
  include_directories: [
    hypha_log_inc,
    hypha_inc,
  ],
  install: true
)
hypha_dep = declare_dependency(
  link_with: [
    hypha,
  ],
  include_directories: [
    hypha_inc,
  ]
)

if gtest_dep.found()
  subdir('test/')
endif

