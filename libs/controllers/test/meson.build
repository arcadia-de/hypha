test_controller_inc = include_directories('include/')
test_controller = static_library(
  'hypha-test-controller',
  [
    'src/controller.c',
  ],
  dependencies: [
    hypha_controller_dep,
    controller_dep,
  ],
  include_directories: [
    test_controller_inc,
  ]
)
