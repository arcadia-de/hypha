pkg_manager_test_exec = executable('hypha-pkg-manager-api-tests',
  sources : [],
  dependencies : [
    gtest_dep,
    pkg_manager_api_dep,
  ],
)

test('GoogleTest Core Features',
  pkg_manager_test_exec,
  protocol: 'gtest',
)
