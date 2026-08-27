hypha_api_inc = include_directories('include/')
api_headers_dep = declare_dependency(
  include_directories: [
    hypha_api_inc,
  ],
)
hypha_api = static_library(
  'hypha-api',
  [
    'src/annotation.c',
    'src/label.c',
    'src/name.c',
  ],
  dependencies: [
    uuid,
  ],
  include_directories: [
    hypha_api_inc,
  ],
  install: false,
)

hypha_api_dep = declare_dependency(
  link_with: hypha_api,
  dependencies: [
    uuid,
  ],
  include_directories: hypha_api_inc,
)
