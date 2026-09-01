pkg_manager_test_exec = executable('hypha-pkg-manager-api-tests',
  sources : [
    'test_package_manager_api.cc',
  ],
  dependencies : [
    gtest_dep,
    pkg_manager_api_dep,
  ],
)

test(
  'Package Manager Api Tests',
  pkg_manager_test_exec,
  protocol: 'gtest',
)
