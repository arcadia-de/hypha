subdir('api/')
subdir('lua-runtime/')

controllers_matrix = {
  'test': {},
  'dir': {},
  'package': {},
  'symlink': {},
  'template': {},
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

base_controller_deps = [
  jansson,
  hypha_api_dep,
  hypha_log_dep,
  hypha_utils_dep,
  hypha_resource_graph_dep,
  hypha_controller_dep,
]
controllers_deps = []
foreach name, cfg : controllers_matrix
  binding_inc = include_directories(name)
  binding_lib = static_library(
    'hypha-' + name + '-controller',
    files(name / name + '_controller.c'),
    include_directories: binding_inc,
    dependencies: base_controller_deps + cfg.get('deps', []),
    override_options: cfg.get('override_opts', []),
  )
  controllers_deps += declare_dependency(
    link_with: binding_lib,
    include_directories: binding_inc,
  )
endforeach
