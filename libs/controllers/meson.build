controller_dep = declare_dependency(
  link_with: [
    hypha_log,
  ],
  dependencies: [
    hypha_api_dep,
    jansson,
    hypha_resource_graph_dep,
    hypha_controller_dep,
  ],
  include_directories: [
    hypha_log_inc,
  ]
)

subdir('test/')

if archive.found()
  subdir('archive/')
endif

subdir('dir/')
subdir('package/')

if git2.found()
  subdir('repository/')
endif

subdir('symlink/')
subdir('template/')
