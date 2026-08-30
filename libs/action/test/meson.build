action_test_exec = executable(
  'test-hypha-action',
  sources : [
    'test_action_log.cc',
  ],
  dependencies : [
    gtest_dep,
    hypha_action_dep,
  ]
)

test(
  'Action Tests',
  action_test_exec,
  protocol: 'gtest',
)
