test_controller_test_exec = executable('hypha-test-controller-tests',
  sources: [
    'test_test_controller.cc',
  ],
  include_directories: [
    binding_inc,
  ],
  dependencies: [
    gtest_dep,
  ] + base_controller_deps + cfg.get('deps', []),
  link_with: binding_lib,
)

test('GoogleTest Core Features',
  test_controller_test_exec,
  protocol: 'gtest',
)
