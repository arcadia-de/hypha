test_utils_exec = executable('hypha-utils-tests',
  sources : [
    'test_expander.cc',
    'test_ignore.cc',
  ],
  include_directories: hypha_utils_inc,
  dependencies : [
    gtest_dep,
    hypha_utils_dep,
  ],
  cpp_args: [
    '-Wno-write-strings',
  ],
  install: false,
)

test(
  'Hypha Utils Tests',
  test_utils_exec,
  protocol: 'gtest'
)
