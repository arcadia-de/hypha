hypha_state_inc = include_directories('include/')
hypha_state = static_library(
  'hypha-state',
  [
    'src/crc32.c',
    'src/state_store.c',
    'src/state_compactor.c',
    'src/state_index.c',
    'src/state_log.c',
  ],
  dependencies: [
    libuv,
    threads,
    hypha_log_dep,
  ],
  include_directories: [
    hypha_state_inc,
  ],
  install: true
)
hypha_state_dep = declare_dependency(
  link_with: [
    hypha_state,
  ],
  include_directories: [
    hypha_state_inc,
  ]
)

if gtest_dep.found()
  subdir('test/')
endif

