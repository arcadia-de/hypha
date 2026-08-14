hypha_log_inc = include_directories('include/')
hypha_log = static_library(
  'hypha-log',
  [
    'src/log.c',
  ],
  dependencies: [
    libuv,
    threads,
  ],
  include_directories: [
    hypha_log_inc,
  ],
  install: true
)
hypha_log_dep = declare_dependency(
  link_with: [
    hypha_log,
  ],
  include_directories: [
    hypha_log_inc,
  ]
)

if gtest_dep.found()
  subdir('test/')
endif

