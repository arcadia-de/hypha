lua_controller_inc = include_directories('include/')
lua_controller = static_library(
  'hypha-controller-runtime-lua',
  [
    'src/lua_controller.c',
  ],
  dependencies: [
    lua54,
    hypha_api_dep,
    hypha_log_dep,
    hypha_controller_dep,
    hypha_resource_graph_dep,
  ],
  include_directories: [
    lua_controller_inc,
  ],
  install: false,
)
lua_controller_dep = declare_dependency(
  link_with: [
    lua_controller,
  ],
  dependencies: [
    lua54,
  ],
  include_directories: [
    lua_controller_inc,
  ]
)

if gtest_dep.found()
  subdir('test/')
endif

