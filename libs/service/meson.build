subdir('api/')

services_matrix = {
  'systemd': {},
}

base_service_deps = [
  hypha_api_dep,
  hypha_log_dep,
  hypha_utils_dep,
  hypha_service_dep,
]
services_deps = []
foreach name, cfg : services_matrix 
  binding_inc = include_directories(name)
  binding_deps = base_service_deps  + cfg.get('deps', [])
  binding_lib = static_library(
    'hypha-' + name,
    files(name / name + '.c'),
    include_directories: binding_inc,
    dependencies: binding_deps,
    override_options: cfg.get('override_opts', []),
  )
  services_deps += declare_dependency(
    link_with: binding_lib,
    dependencies: binding_deps,
    include_directories: binding_inc,
  )
endforeach
