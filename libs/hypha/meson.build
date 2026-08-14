hypha_inc_dir = meson.current_source_dir() / 'include'
hypha_inc = include_directories('include/')
hypha = static_library(
  'hypha',
  [
    'src/hypha.c',
    'src/event.c',
    'src/decorator.c',
    'src/orchestrator.c',
    'src/orchestrator_lua.c',
    'src/reconcile.c',
    'src/history_log.c',
    'src/query_schema_resources.c',
    'src/package_managers.c',
  ],
  dependencies: [
    libuv,
    threads,
    lua54,
    wamr,
    gvc,

    hypha_api_dep,
    hypha_query_dep,
    hypha_state_dep,
    hypha_utils_dep,
    repository_controller_dep,
    hypha_controller_dep,
    hypha_resource_graph_dep,
    pkg_manager_api_dep,
    all_pkg_manager_deps,
  ],
  link_with: [
    hypha_log,
    hypha_utils,
    test_controller,
    archive_controller,
    dir_controller,
    package_controller,
    symlink_controller,
    template_controller,
  ],
  include_directories: [
    hypha_log_inc,
    hypha_inc,
    test_controller_inc,
    archive_controller_inc,
    dir_controller_inc,
    package_controller_inc,
    repository_controller_inc,
    symlink_controller_inc,
    template_controller_inc,
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

