repository_controller_inc = include_directories('include/')
repository_controller = static_library(
  'hypha-repository-controller',
  [
    'src/controller.c',
  ],
  dependencies: [
    hypha_controller_dep,
    controller_dep,
    git2,
  ],
  include_directories: [
    repository_controller_inc,
  ]
)
repository_controller_dep = declare_dependency(
  link_with: repository_controller,
  dependencies: [
    git2,
  ]
)
