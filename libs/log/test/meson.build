log_test_exec = executable('hypha-log-tests',
  sources : [
    'test_log.cc',
  ],
  dependencies: [
    gtest_dep,
    hypha_log_dep,
  ]
)

test(
  'Hypha Log Tests',
  log_test_exec,
  protocol: 'gtest',
)
