hypha_query_inc = include_directories('include/')
hypha_query_deps = [
  libuv,
  threads,
  hypha_api_dep,
]
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
  dependencies: hypha_query_deps,
  include_directories: hypha_query_inc,
  install: false,
)
hypha_query_dep = declare_dependency(
  link_with: hypha_query,
  dependencies: hypha_query_deps,
  include_directories: hypha_query_inc,
)

if gtest_dep.found()
  subdir('test/')
endif

