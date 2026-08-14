hypha_utils_inc = include_directories('include/')
hypha_utils_deps = [
  hypha_log_dep,
]
hypha_utils = static_library(
  'hypha-utils',
  [
    'src/process.c',
    'src/expander.c',
    'src/glob.c',
    'src/crc32.c',
    'src/bitset.c',
  ],
  dependencies: hypha_utils_deps,
  include_directories: hypha_utils_inc,
  install: false,
)
hypha_utils_dep = declare_dependency(
  link_with: hypha_utils,
  include_directories: hypha_utils_inc,
)

if gtest_dep.found()
  subdir('test/')
endif

