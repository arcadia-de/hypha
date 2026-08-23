all_hypha_controller_deps = [
  libuv,
  threads,
  hypha_api_dep,
  hypha_log_dep,
  hypha_resource_graph_dep,
  hypha_validation_dep,
  hypha_planner_dep,
  hypha_service_dep,
]

if enable_profiling
  if tracy_dep.found()
    all_hypha_controller_deps += tracy_dep
  endif
endif

hypha_controller_inc = include_directories('include/')
hypha_controller = static_library(
  'hypha-controller',
  [
    'src/controller.c',
  ],
  dependencies: all_hypha_controller_deps,
  include_directories: [
    hypha_controller_inc,
  ],
  install: false,
)
hypha_controller_dep = declare_dependency(
  link_with: [
    hypha_controller,
  ],
  include_directories: [
    hypha_controller_inc,
  ]
)

if gtest_dep.found()
  subdir('test/')
endif

