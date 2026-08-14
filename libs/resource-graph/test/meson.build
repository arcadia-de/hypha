resource_graph_test_exec = executable('hypha-resource-graph-tests',
  sources : [

  ],
  link_with: [
    hypha_resource_graph,
  ],
  dependencies : [
    gtest_dep
  ],
)

test('GoogleTest Core Features',
  resource_graph_test_exec,
  protocol: 'gtest',
)
