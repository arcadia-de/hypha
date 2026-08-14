dir_controller_inc =  include_directories('include')
dir_controller = static_library(
  'hypha-dir-controller',
  [
    'src/controller.c',
  ],
  dependencies: [
    hypha_controller_dep,
    controller_dep,
  ],
  include_directories: [
    dir_controller_inc,
  ]
)
