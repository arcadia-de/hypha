hypha_state_test_exec = executable('hypha-state-tests',
  sources: [

  ],
  dependencies: [
    gtest_dep,
    hypha_state_dep,
  ],
)

test('GoogleTest Core Features',
  hypha_state_test_exec,
  protocol: 'gtest',
)
