hypha_utils_inc = include_directories('include/')
hypha_utils_deps = [
  hypha_log_dep,
  hypha_api_dep,
]
hypha_utils = static_library(
  'hypha-utils',
  [
    'src/process.c',
    'src/platform.c',
    'src/expander.c',
    'src/ignore.c',
    'src/glob.c',
    'src/crc32.c',
    'src/bitset.c',
    'src/env.c',
  ],
  c_args: ['-Wno-unused-variable'],
  dependencies: hypha_utils_deps,
  include_directories: hypha_utils_inc,
  install: false,
)
hypha_utils_dep = declare_dependency(
  link_with: hypha_utils,
  dependencies: hypha_utils_deps,
  include_directories: hypha_utils_inc,
)

if gtest_dep.found()
  subdir('test/')
endif

