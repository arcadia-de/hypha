hypha_events_inc = include_directories('include/')
hypha_events_deps = [
  hypha_api_dep,
  hypha_log_dep,
]
hypha_events = static_library(
  'hypha-events',
  [
    'src/event.c',
    'src/event_trie.c',
    'src/event_bus.c',
  ],
  dependencies: hypha_events_deps,
  include_directories: hypha_events_inc,
  install: false,
)
hypha_events_dep = declare_dependency(
  link_with: hypha_events,
  dependencies: hypha_events_deps,
  include_directories: hypha_events_inc,
)

if gtest_dep.found()
  subdir('test/')
endif
