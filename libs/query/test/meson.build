query_test_exec = executable('hypha-query-tests',
  sources : [

  ],
  link_with: [
    hypha_log,
  ],
  dependencies : [
    gtest_dep
  ],
)

test('GoogleTest Core Features',
  log_test_exec,
  protocol: 'gtest',
)
