hypha_state_test_exec = executable('hypha-state-tests',
  sources: [
    'test_state.cc',
  ],
  dependencies: [
    gtest_dep,
    hypha_state_dep,
  ],
)

test(
  'Hypha State Tests',
  hypha_state_test_exec,
  protocol: 'gtest',
)
