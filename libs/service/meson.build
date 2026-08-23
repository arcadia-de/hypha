hypha_service_inc = include_directories('include/')
hypha_service_deps = [
  libuv,
  threads,
  hypha_api_dep,
  hypha_log_dep,
  hypha_utils_dep,
  hypha_resource_graph_dep,
]
hypha_service = static_library(
  'hypha-service',
  [
    'src/systemd.c',
  ],
  dependencies: hypha_service_deps,
  include_directories: [
    hypha_service_inc,
  ],
  install: false,
)
hypha_service_dep = declare_dependency(
  link_with: hypha_service,
  dependencies: hypha_service_deps,
  include_directories: [
    hypha_service_inc,
  ],
)

if gtest_dep.found()
  subdir('test/')
endif

