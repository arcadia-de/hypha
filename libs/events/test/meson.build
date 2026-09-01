hypha_events_test_exec = executable(
  'hypha-events-tests',
  files(
    'test_event.cc',
  ),
  dependencies: [
    gtest_dep,
    hypha_api_dep,
    hypha_events_dep,
  ],
)

test(
  'Events Tests',
  hypha_events_test_exec,
  protocol: 'gtest',
)
