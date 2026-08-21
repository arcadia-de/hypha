hypha_history_inc = include_directories('include/')
hypha_history_deps = [
  libuv,
  threads,
  hypha_api_dep,
  hypha_state_dep,
]
hypha_history = static_library(
  'hypha-history',
  [
    'src/history.c',
  ],
  dependencies: hypha_history_deps,
  include_directories: hypha_history_inc,
  install: false,
)
hypha_history_dep = declare_dependency(
  link_with: hypha_history,
  dependencies: hypha_history_deps,
  include_directories: hypha_history_inc,
)

if gtest_dep.found()
  subdir('test/')
endif

