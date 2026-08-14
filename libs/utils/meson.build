hypha_utils_inc = include_directories('include/')
hypha_utils = static_library(
  'hypha-utils',
  [
    'src/process.c',
  ],
  dependencies: [
    hypha_log_dep,
  ],
  include_directories: [
    hypha_utils_inc,
  ],
  install: true
)
hypha_utils_dep = declare_dependency(
  link_with: [
    hypha_utils,
  ],
  include_directories: [
    hypha_utils_inc,
  ]
)

if gtest_dep.found()
  subdir('test/')
endif

