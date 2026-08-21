hypha_planner_inc = include_directories('include/')
hypha_planner_deps = [
  libuv,
  threads,
  hypha_api_dep,
  hypha_log_dep,
  hypha_utils_dep,
]
hypha_planner = static_library(
  'hypha-planner',
  [
    'src/planner.c',
    'src/planned_action.c',
  ],
  dependencies: hypha_planner_deps,
  include_directories: [
    hypha_planner_inc,
  ],
  install: false,
)
hypha_planner_dep = declare_dependency(
  link_with: hypha_planner,
  dependencies: hypha_planner_deps,
  include_directories: [
    hypha_planner_inc,
  ],
)

if gtest_dep.found()
  subdir('test/')
endif

