subdir('api')
subdir('lua-runtime')
subdir('wasm-runtime')

controllers_matrix = {
  'test': {},
  'directory': {},
  'package': {},
  'symlink': {},
  'template': {},
  'package_manager': {},
}

if archive.found()
  controllers_matrix += {
    'archive': {
      'deps': [
        archive,
      ],
    },
  }
endif

if git2.found()
  controllers_matrix += {
    'repository': {
      'deps': [
        git2,
      ],
    },
  }
endif

base_c_args = [
  '-Wno-unused-function',
]
base_controller_deps = [
  jansson,
  hypha_api_dep,
  hypha_log_dep,
  hypha_utils_dep,
  hypha_resource_graph_dep,
  hypha_controller_dep,
  hypha_planner_dep,
  hypha_action_dep,
  hypha_validation_dep,
  hypha_state_dep,
]
controllers_deps = []
foreach name, cfg : controllers_matrix
  binding_inc = include_directories(name / 'include')
  binding_lib = static_library(
    'hypha-' + name + '-controller',
    files(name / 'src' / name + '_controller.c'),
    include_directories: binding_inc,
    dependencies: base_controller_deps + cfg.get('deps', []),
    c_args: base_c_args + cfg.get('c_args', []),
    override_options: cfg.get('override_opts', []),
  )
  controllers_deps += declare_dependency(
    link_with: binding_lib,
    include_directories: binding_inc,
  )
endforeach

subdir('all')
