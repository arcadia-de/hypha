hypha_log_inc = include_directories('include/')
hypha_log_deps = [
  libuv,
  threads,
]
hypha_log = static_library(
  'hypha-log',
  [
    'src/log.c',
  ],
  dependencies: hypha_log_deps,
  include_directories: hypha_log_inc,
  install: false,
)
hypha_log_dep = declare_dependency(
  link_with: hypha_log,
  dependencies: hypha_log_deps,
  include_directories: hypha_log_inc,
)
