pkg_manager_api_inc = include_directories('include/')
pkg_manager_api = static_library(
  'hypha-pkg-manager-api',
  [
    'src/package_manager.c',
  ],
  dependencies: [
    hypha_api_dep,
    hypha_log_dep,
    hypha_utils_dep,
  ],
  c_args: ['-Wno-unused-variable'],
  include_directories: [
    pkg_manager_api_inc,
  ],
  install: false,
)
pkg_manager_api_dep = declare_dependency(
  link_with: [
    pkg_manager_api,
  ],
  include_directories: [
    pkg_manager_api_inc,
  ]
)

if gtest_dep.found()
  subdir('test/')
endif

