history_test_exec = executable('hypha-history-tests',
  sources : [
    'test_history.cc',
  ],
  dependencies: [
    gtest_dep,
    hypha_history_dep,
  ]
)

test(
  'Hypha History Tests',
  history_test_exec,
  protocol: 'gtest',
)
