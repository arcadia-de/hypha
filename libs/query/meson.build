hypha_query_inc = include_directories('include/')
hypha_query = static_library(
  'hypha-query',
  [
    'src/query.c',
    'src/query_ast.c',
    'src/query_executor.c',
    'src/query_lexer.c',
    'src/query_parser.c',
    'src/query_result.c',
  ],
  dependencies: [
    libuv,
    threads,
  ],
  include_directories: [
    hypha_query_inc,
  ],
  install: true
)
hypha_query_dep = declare_dependency(
  link_with: [
    hypha_query,
  ],
  include_directories: [
    hypha_query_inc,
  ]
)

if gtest_dep.found()
  subdir('test/')
endif

