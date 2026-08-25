hypha_controllers_deps = [
  hypha_controller_dep,
]
hypha_controllers_deps += controllers_deps

hypha_controllers_inc = include_directories('include/')
hypha_controllers = static_library(
  'hypha-all-controllers',
  [
    'src' / 'controllers.c',
  ],
  dependencies: hypha_controllers_deps,
  include_directories: [
    hypha_controllers_inc,
  ],
  install: false,
)
hypha_controllers_dep = declare_dependency(
  link_with: hypha_controllers,
  dependencies: hypha_controllers_deps,
  include_directories: [
    hypha_controllers_inc,
  ]
)

# if gtest_dep.found()
#   subdir('test/')
# endif
