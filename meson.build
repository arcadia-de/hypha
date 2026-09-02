project(
  'hypha',
  ['c', 'cpp'],
  version: files('VERSION'),
  default_options: ['c_std=c23,c2x'],
  meson_version: '>= 1.10.0',
)
add_project_arguments('-D_XOPEN_SOURCE=700', language: 'c')
add_project_arguments(
  '-DHYPHA_VERSION="' + meson.project_version() + '"',
  language: 'c',
)

enable_profiling = get_option('enable_profiling')

if get_option('buildtype') == 'debug'
  message('debug mode is enabled')
  add_project_arguments('-DHYPHA_DEBUG', language: ['c', 'cpp'])
endif

threads = dependency('threads', required: true)
libuv = dependency('libuv', version: '>=1.0.0', required: true)
lua54 = dependency('lua54', version: '=5.4.8', required: true)
jansson = dependency('jansson', version: '>=2.15.0', required: true)
sodium = dependency('libsodium', version: '>=1.0.22', required: true)
uuid = dependency('uuid', version: '>=2.42.2', required: true)
xxhash = dependency('libxxhash', version: '>=0.8.3', required: true)

all_hypha_deps = [
  threads,
  libuv,
  lua54,
  jansson,
  sodium,
  uuid,
  xxhash,
]

archive = dependency('libarchive', version: '>=3.8.8', required: false)
if archive.found()
  all_hypha_deps += archive
  add_project_arguments('-DHYPHA_HAS_ARCHIVE', language: ['c', 'cpp'])
endif

git2 = dependency('libgit2', version: '>=1.9.4', required: false)
if git2.found()
  all_hypha_deps += git2
  add_project_arguments('-DHYPHA_HAS_GIT2', language: ['c', 'cpp'])
endif

curl = dependency('libcurl', version: '>=8.21.0', required: false)
if curl.found()
  all_hypha_deps += curl
  add_project_arguments('-DHYPHA_HAS_CURL', language: ['c', 'cpp'])
endif

gvc = dependency('libgvc', version: '>= 15.1.0', required: false)
if gvc.found()
  message('graphviz is enabled')
  add_project_arguments('-DHYPHA_GRAPHVIZ_ENABLED', language: ['c', 'cpp'])
  all_hypha_deps += gvc
endif

wamr = dependency('libiwasm', version: '>=2.2.0', required: false)
if wamr.found()
  message('wamr is enabled')
  add_project_arguments('-DHYPHA_WAMR_ENABLED', language: ['c', 'cpp'])
  all_hypha_deps += wamr
endif

tracy_dep = dependency('tracy', required: false)
if enable_profiling
  if tracy_dep.found()
    message('profiling is enabled with tracy')
    all_hypha_deps += tracy_dep
    add_project_arguments('-DHYPHA_ENABLE_PROFILING', language: ['c', 'cpp'])
  endif
endif

gtest_dep = dependency('gtest', main: true, required: false)

subdir('libs/')

cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required: false)
pkg = import('pkgconfig')

pkg.generate(
  name: 'hypha',
  version: meson.project_version(),
  description: 'Hypha Core Libraries and Dependencies',
  libraries: [
    hypha,
    hypha_controller,
    hypha_resource_graph,
    hypha_query,
    hypha_state,
    hypha_log,
    hypha_utils,
  ],
  requires: all_hypha_deps,
  libraries_private: [
    m_dep,
  ],
  subdirs: [
    'libs/hypha/include/',
  ],
)

python3 = import('python').find_installation()

go = find_program('go', required: true)

go_build_env = environment()
go_build_env.set('CGO_ENABLED', '1')
go_build_env.set('CGO_LDFLAGS', '-Wl,-export-dynamic -Wl,--whole-archive -Wl,--no-whole-archive')
go_build_env.prepend('PKG_CONFIG_PATH', meson.project_source_root() / 'build' / 'meson-uninstalled')

if get_option('buildtype') == 'debug'
  hypha = custom_target(
    'hypha',
    output: 'hypha',
    input:  'cmd' / 'hypha' / 'main.go',
    env : go_build_env,
    command: [
      python3, '-c',
      'import os, sys, subprocess; ' +
      'os.path.exists(sys.argv[1]) and os.remove(sys.argv[1]); ' +
      'sys.exit(subprocess.run(sys.argv[2:]).returncode)',
      '@OUTPUT@',

      go,
      'build',
      '-ldflags=-linkmode=external',
      '-o', '@OUTPUT@',
      '@INPUT@',
    ],
    build_by_default: true,
  )
else
  hypha = custom_target(
    'hypha',
    output: 'hypha',
    input:  'cmd' / 'hypha' / 'main.go',
    env : go_build_env,
    command: [
      go,
      'build',
      '-ldflags=-linkmode=external',
      '-s', '-w',
      '-o', '@OUTPUT@',
      '@INPUT@',
    ],
    build_by_default: true,
  )
endif

go_test_env = environment()
go_test_env.prepend('PKG_CONFIG_PATH', meson.project_source_root() / 'build' / 'meson-uninstalled')

test(
  'Hypha CGo Tests',
  go,
  workdir: meson.project_source_root(),
  env: go_test_env,
  args: [
    'test',
    './internal/hypha',
  ],
)

test(
  'Hypha CLI Tests',
  go,
  workdir: meson.project_source_root(),
  env: go_test_env,
  args: [
    'test',
    './internal/cli',
  ],
)
