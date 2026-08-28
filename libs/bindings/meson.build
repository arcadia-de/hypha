bindings_matrix = {
  'host': {
    'deps': [
      hypha_api_dep,
      hypha_utils_dep,
    ],
  },
  'sources': {
    'deps': [
      hypha_api_dep,
      hypha_utils_dep,
    ],
  },
  'env': {
    'deps': [
      hypha_utils_dep,
      hypha_api_dep,
    ],
  },
  'events': {
    'deps': [
      hypha_api_dep,
      hypha_events_dep,
    ],
  },
  'runtime': {
    'deps': [
      hypha_utils_dep,
      api_headers_dep,
    ],
  },
  'log': {
    'deps': [
      hypha_api_dep,
    ],
  },
  'packages': {
    'deps': [
      hypha_api_dep,
      pkg_manager_api_dep,
      pkg_manager_deps,
    ],
    'c_args': ['-Wno-unused-variable'],
  },
  #TODO(@s0cks):
  # 'query': {
  #   'deps': [
  #     hypha_api_dep,
  #     hypha_utils_dep,
  #     hypha_resource_graph_dep,
  #     hypha_query_dep,
  #     hypha_state_dep,
  #     hypha_planner_dep,
  #     hypha_validation_dep,
  #     hypha_service_dep,
  #     hypha_events_dep,
  #     hypha_controller_dep,
  #     hypha_action_dep,
  #   ],
  # },
}

base_override_opts = [
  'b_lundef=false',
]
base_link_args = [
  '-Wl,--allow-shlib-undefined',
  '-Wl,--unresolved-symbols=ignore-all'
]
base_binding_deps = [
  lua54,
  hypha_log_dep,
]
bindings_deps = []
foreach name, cfg : bindings_matrix
  binding_inc = include_directories(name / 'include')
  binding_sources = [
    name / 'src' / name + '_bindings.c',
  ]
  binding_sources += cfg.get('sources', [])
  binding_lib = static_library(
    name,
    binding_sources,
    include_directories: binding_inc,
    install: false,
    link_args: base_link_args + cfg.get('link_args', []),
    c_args: cfg.get('c_args', []),
    dependencies: base_binding_deps + cfg.get('deps', []),
    override_options: base_override_opts + cfg.get('override_opts', []),
  )
  bindings_deps += declare_dependency(
    link_with: binding_lib,
    include_directories: binding_inc,
  )
endforeach
