bindings_matrix = {
  'host': {
    'deps': [],
    'override_opts': [],
  },
  'runtime': {
    'deps': [
      hypha_utils_dep,
    ],
    'override_opts': [],
  },
  'jsonnet': {
    'deps': [],
    'override_opts': [
      'b_lundef=false',
    ],
  },
  'log': {
    'deps': [
      hypha_log_dep,
    ],
    'override_opts': [],
  },
  'packages': {
    'deps': [
      hypha_api_dep,
      hypha_log_dep,
      pkg_manager_api_dep,
      all_pkg_manager_deps,
    ],
    'override_opts': [],
  },
  'resource': {
    'deps': [
      hypha_dep,
      hypha_resource_graph_dep,
      hypha_query_dep,
      hypha_controller_dep,
    ],
    'override_opts': [],
  },
  'template': {
    'deps': [],
    'override_opts': [
      'b_lundef=false',
    ],
  },
}

base_binding_deps = [
  lua54,
  hypha_api_dep,
]
all_bindings_deps = []
foreach name, cfg : bindings_matrix
  binding_inc = include_directories(name)
  binding_lib = shared_library(
    name,
    files(name / name + '_bindings.c'),
    build_subdir: 'hypha',
    name_prefix: '',
    include_directories: [
      binding_inc,
    ],
    dependencies: base_binding_deps + cfg.get('deps'),
    override_options: cfg.get('override_opts'),
  )
  all_bindings_deps += declare_dependency(
    link_with: [
      binding_lib,
    ],
    include_directories: [
      binding_inc,
    ],
  )
endforeach
