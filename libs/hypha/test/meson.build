test_exec = executable('hypha-tests',
  sources : [
    'test_crc32.cc',
  ],
  link_with: [
    hypha,
  ],
  dependencies : [
    hypha_api_dep,
    gtest_dep
  ]
)

test('GoogleTest Core Features', test_exec, protocol : 'gtest')
