archive_controller_inc = include_directories('include/')
archive_controller = static_library(
  'hypha-archive-controller',
  [
    'src/controller.c',
  ],
  dependencies: [
    hypha_controller_dep,
    hypha_resource_graph_dep,
    controller_dep,
    archive,
  ],
  include_directories: [
    archive_controller_inc,
  ]
)
