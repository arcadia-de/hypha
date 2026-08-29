delta_test_exec = executable('hypha-tests',
  sources : [
    'test_delta_log.cc',
  ],
  dependencies : [
    gtest_dep,
    delta_dep,
  ]
)

test(
  'Delta Log Tests',
  delta_test_exec,
  protocol: 'gtest',
)
