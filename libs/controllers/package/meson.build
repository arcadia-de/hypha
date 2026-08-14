package_controller_inc = include_directories('include/')
package_controller = static_library(
  'hypha-package-controller',
  [
    'src/controller.c',
  ],
  dependencies: [
    hypha_controller_dep,
    controller_dep,
    all_pkg_manager_deps,
  ],
  include_directories: [
    package_controller_inc,
  ]
)
