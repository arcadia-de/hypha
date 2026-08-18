hypha_api_inc = include_directories('include/')

hypha_api = static_library(
  'hypha-api',
  [
    'src/annotation.c',
  ],
  include_directories: [
    hypha_api_inc,
  ],
  install: false,
)

hypha_api_dep = declare_dependency(
  link_with: hypha_api,
  include_directories: hypha_api_inc,
)
