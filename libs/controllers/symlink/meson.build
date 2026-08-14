symlink_controller_inc = include_directories('include/')
symlink_controller = static_library(
  'hypha-symlink-controller',
  [
    'src/controller.c',
  ],
  dependencies: [
    hypha_controller_dep,
    controller_dep,
    jansson,
  ],
  include_directories: [
    symlink_controller_inc,
  ]
)
