bindings_matrix = {
  'host': {
    'deps': [
      hypha_utils_dep,
    ],
  },
  'sources': {
    'deps': [
      hypha_utils_dep,
    ],
  },
  'env': {
    'deps': [
      hypha_utils_dep,
    ],
  },
  'events': {
    'deps': [
      hypha_events_dep,
    ],
  },
  'runtime': {
    'deps': [
      hypha_utils_dep,
      lua_controller_dep,
      hypha_controller_dep,
      hypha_resource_graph_dep,
      hypha_validation_dep,
      hypha_service_dep,
      hypha_action_dep,
      hypha_planner_dep,
    ],
  },
  'log': {},
  'packages': {
    'deps': [
      hypha_api_dep,
      pkg_manager_api_dep,
      pkg_manager_deps,
    ],
  },
  'query': {
    'deps': [
      hypha_dep,
      hypha_utils_dep,
      hypha_resource_graph_dep,
      hypha_query_dep,
      hypha_planner_dep,
      hypha_validation_dep,
      hypha_service_dep,
      hypha_events_dep,
      hypha_controller_dep,
      hypha_action_dep,
    ],
  },
}

base_override_opts = [
  'b_lundef=false',
]
base_binding_deps = [
  lua54,
  hypha_api_dep,
  hypha_log_dep,
]
bindings_deps = []
foreach name, cfg : bindings_matrix
  binding_inc = include_directories(name)
  binding_lib = shared_library(
    name,
    files(name / name + '_bindings.c'),
    build_subdir: 'hypha',
    name_prefix: '',
    include_directories: binding_inc,
    install: false,
    dependencies: base_binding_deps + cfg.get('deps', []),
    override_options: base_override_opts + cfg.get('override_opts', []),
  )
  bindings_deps += declare_dependency(
    link_with: binding_lib,
    include_directories: binding_inc,
  )
endforeach
