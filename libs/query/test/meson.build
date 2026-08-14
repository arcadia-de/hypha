query_test_exec = executable('hypha-query-tests',
  sources : [
    'test_query.cc',
  ],
  dependencies : [
    gtest_dep,
    hypha_query_dep,
  ],
)

test(
  'Hypha Query Tests',
  query_test_exec,
  protocol: 'gtest',
)
