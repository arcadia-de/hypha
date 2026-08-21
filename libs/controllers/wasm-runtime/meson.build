wasm_controller_inc = include_directories('include/')
wasm_controller_deps = [
  hypha_api_dep,
  hypha_log_dep,
  hypha_controller_dep,
  hypha_resource_graph_dep,
]
wasm_controller = static_library(
  'hypha-controller-runtime-wasm',
  [
    'src/wasm_controller.c',
  ],
  dependencies: wasm_controller_deps,
  include_directories: [
    wasm_controller_inc,
  ],
  install: false,
)
wasm_controller_dep = declare_dependency(
  link_with: [
    wasm_controller,
  ],
  dependencies: wasm_controller_deps,
  include_directories: [
    wasm_controller_inc,
  ]
)

# if gtest_dep.found()
#   subdir('test/')
# endif
#
