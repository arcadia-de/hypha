template_controller_inc = include_directories('include/')
template_controller = static_library(
  'hypha-template-controller',
  [
    'src/controller.c',
  ],
  dependencies: [
    hypha_controller_dep,
    controller_dep,
  ],
  include_directories: [
    template_controller_inc,
  ]
)
