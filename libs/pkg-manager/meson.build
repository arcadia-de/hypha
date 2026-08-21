subdir('api/')
pkg_manager_matrix = {
  'luarocks': {
    'deps': [],
    'override_opts': [],
  },
  'apt': {
    'deps': [],
    'override_opts': [],
  },
  'pacman': {
    'deps': [],
    'override_opts': [],
  },
  'yay': {
    'deps': [],
    'override_opts': [],
  },
  'paru': {
    'deps': [],
    'override_opts': [],
  },
  'asdf': {
    'deps': [],
    'override_opts': [],
  },
  'brew': {
    'deps': [],
    'override_opts': [],
  },
  'cargo': {
    'deps': [],
    'override_opts': [],
  },
  'dnf': {
    'deps': [],
    'override_opts': [],
  },
  'eget': {
    'deps': [],
    'override_opts': [],
  },
  'flatpak': {
    'deps': [],
    'override_opts': [],
  },
  'go': {
    'deps': [],
    'override_opts': [],
  },
}

base_pkg_manager_deps = [
  hypha_api_dep,
  hypha_log_dep,
  hypha_utils_dep,
  pkg_manager_api_dep,
]
pkg_manager_deps = []
foreach name, cfg : pkg_manager_matrix
  pkg_manager_inc = include_directories(name)
  pkg_manager_lib = static_library(
    name,
    files(name / name + '_pkg_manager.c'),
    include_directories: [
      pkg_manager_inc,
    ],
    c_args: [ '-Wno-unused-variable' ],
    dependencies: base_pkg_manager_deps + cfg.get('deps'),
    override_options: cfg.get('override_opts'),
  )
  pkg_manager_deps += declare_dependency(
    link_with: [
      pkg_manager_lib,
    ],
    include_directories: [
      pkg_manager_inc,
    ],
  )
endforeach

subdir('all/')
