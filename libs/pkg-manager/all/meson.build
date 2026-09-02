pkg_managers_deps = [
  hypha_api_dep,
  hypha_log_dep,
  hypha_utils_dep,
  pkg_manager_api_dep,
] + pkg_manager_deps
pkg_managers = static_library(
  'hypha-pkg-managers',
  [
    'package_managers.c',
  ],
  dependencies: pkg_managers_deps,
  c_args: [
    '-Wno-unused',
  ],
  install: false,
)
pkg_managers_dep = declare_dependency(
  dependencies: pkg_manager_deps,
  link_with: pkg_managers,
)
