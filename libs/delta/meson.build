delta_inc = include_directories('include')
delta_deps = [
  hypha_api_dep,
  hypha_log_dep,
  hypha_utils_dep,
]
delta = static_library(
  'hypha-delta',
  [
    'src' / 'delta_log.c',
  ],
  dependencies: delta_deps,
  include_directories: [
    delta_inc,
  ],
  install: false,
)
delta_dep = declare_dependency(
  link_with: delta,
  dependencies: delta_deps,
  include_directories: [
    delta_inc,
  ],
)

if gtest_dep.found()
  subdir('test')
endif
