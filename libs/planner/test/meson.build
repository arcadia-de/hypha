planner_test_exec = executable('hypha-planner-tests',
  sources : [
    'test_planner.cc',
  ],
  dependencies : [
    gtest_dep,
    hypha_planner_dep,
  ],
)

test(
  'Hypha planner Tests',
  planner_test_exec,
  protocol: 'gtest',
)
