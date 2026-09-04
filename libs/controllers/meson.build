subdir('api')
subdir('lua-runtime')
subdir('wasm-runtime')

controllers_matrix = {
  'test': {
    'test_name': 'Test Controller Tests',
    'sources': [
      'test_spec.c',
    ],
    'has_tests': true,
  },
  'directory': {
    'test_name': 'Directory Controller Tests',
    'sources': [
      'directory_spec.c',
    ],
    'has_tests': true,
  },
  'package': {
    'test_name': 'Package Controller Tests',
    'sources': [
      'package_spec.c',
    ],
    'deps': [
      pkg_manager_api_dep,
    ],
    'has_tests': true,
  },
  'symlink': {
    'test_name': 'Symlink Controller Tests',
    'sources': [
      'symlink_spec.c',
    ],
    'has_tests': true,
  },
  'template': {
    'test_name': 'Template Controller Tests',
    'sources': [
      'template_spec.c',
    ],
    'has_tests': true,
  },
  'package_manager': {
    'sources': [
      'package_manager_spec.c',
    ],
    'deps': [
      pkg_manager_api_dep,
    ],
    'test_name': 'Package Manager Controller Tests',
    'has_tests': true,
  },
  'task': {
    'test_name': 'Task Controller Tests',
    'sources': [
       'task_spec.c',
       'exec_spec.c',
    ],
    'has_tests': true,
  },
}

if archive.found()
  controllers_matrix += {
    'archive': {
      'deps': [
        archive,
      ],
      'test_name': 'Archive Controller Tests',
      'has_tests': true,
    },
  }
endif

if git2.found()
  controllers_matrix += {
    'repository': {
      'deps': [
        git2,
      ],
      'test_name': 'Repository Controller Tests',
      'has_tests': true,
    },
  }
endif

if curl.found()
  controllers_matrix += {
    'download': {
      'sources': [
        'download_spec.c',
      ],
      'deps': [
        curl,
        sodium,
      ],
      'test_name': 'Download Controller Tests',
      'has_tests': true,
    },
  }
endif

base_c_args = [
  '-Wno-unused',
]
base_cpp_args = [
  '-Wno-unused',
]
base_controller_deps = [
  jansson,
  xxhash,
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
  binding_sources = [
    name / 'src' / name + '_controller.c',
  ]
  foreach source : cfg.get('sources', [])
    binding_sources += name / 'src' / source
  endforeach
  binding_deps = base_controller_deps + cfg.get('deps', [])
  binding_lib = static_library(
    'hypha-' + name + '-controller',
    binding_sources,
    include_directories: binding_inc,
    dependencies: binding_deps,
    c_args: base_c_args + cfg.get('c_args', []),
    override_options: cfg.get('override_opts', []),
  )
  binding_dep = declare_dependency(
    link_with: binding_lib,
    dependencies: binding_deps,
    include_directories: binding_inc,
  )
  controllers_deps += binding_dep
  if cfg.get('has_tests', false) and gtest_dep.found()
    test_sources = [
      name / 'test' / 'test_' + name + '_controller.cc',
    ]
    foreach source : cfg.get('test_sources', [])
      test_sources += 'test' / source
    endforeach
    controller_test_exec = executable(
      'test-' + name + '-ctrl',
      test_sources,
      c_args: base_c_args + cfg.get('c_args', []),
      cpp_args: base_cpp_args + cfg.get('cpp_args', []),
      dependencies: [
        gtest_dep,
        binding_dep,
      ],
    )
    test(
      cfg.get('test_name', name + ' Controller Tests'),
      controller_test_exec,
      protocol: 'gtest',
    )
  endif
endforeach

subdir('all')
