resource_graph_test_exec = executable('hypha-resource-graph-tests',
  sources : [
    'test_resource.cc',
    'test_resource_bootstrap.cc',
  ],
  dependencies : [
    gtest_dep,
    hypha_resource_graph_dep,
  ],
)

test(
  'Hypha Resource Graph Tests',
  resource_graph_test_exec,
  protocol: 'gtest',
)
