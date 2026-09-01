controller_test_exec = executable('hypha-controller-tests',
  sources : [
    'test_controller_api.cc',
  ],
  dependencies : [
    gtest_dep,
    hypha_controller_dep,
  ],
)

test(
  'Controller Api Tests',
  controller_test_exec,
  protocol: 'gtest',
)
