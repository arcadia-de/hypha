hypha_validation_inc = include_directories('include/')
hypha_validation_deps = [
  libuv,
  threads,
  hypha_api_dep,
  hypha_log_dep,
  hypha_utils_dep,
]
hypha_validation = static_library(
  'hypha-validation',
  [
    'src/validation_log.c',
  ],
  dependencies: hypha_validation_deps,
  include_directories: [
    hypha_validation_inc,
  ],
  install: false,
)
hypha_validation_dep = declare_dependency(
  link_with: hypha_validation,
  dependencies: hypha_validation_deps,
  include_directories: [
    hypha_validation_inc,
  ],
)

if gtest_dep.found()
  subdir('test/')
endif

