hypha_controller_inc = include_directories('include/')
hypha_controller = static_library(
  'hypha-controller',
  [
    'src/controller.c',
  ],
  dependencies: [
    libuv,
    threads,
    hypha_api_dep,
    hypha_log_dep,
    hypha_resource_graph_dep,
  ],
  include_directories: [
    hypha_controller_inc,
  ],
  install: true
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

