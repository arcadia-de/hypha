project(
  'hypha',
  [ 'c', 'cpp' ],
  version: '1.0',
  default_options: ['c_std=c23'],
  meson_version: '>= 1.10.0',
)
add_project_arguments('-D_XOPEN_SOURCE=700', language: 'c')

enable_profiling = get_option('enable_profiling')

if get_option('buildtype') == 'debug'
  message('debug mode is enabled')
  add_project_arguments('-DHYPHA_DEBUG', language: [ 'c', 'cpp' ])
endif

threads = dependency('threads', required: true)
libuv = dependency('libuv', version: '>=1.0.0', required: true)
lua54 = dependency('lua54', version: '=5.4.8', required: true)
jansson = dependency('jansson', version: '>=2.15.0', required: true)
sodium = dependency('libsodium', version: '>=1.0.22', required: true)
uuid = dependency('uuid', version: '>=2.42.2', required: true)

all_hypha_deps = [
  threads,
  libuv,
  lua54,
  jansson,
  sodium,
  uuid,
]

archive = dependency('libarchive', version: '>=3.8.8', required: false)
if archive.found()
  all_hypha_deps += archive
endif

git2 = dependency('libgit2', version: '>=1.9.4', required: false)
if git2.found()
  all_hypha_deps += git2
endif

gvc = dependency('libgvc', version: '>= 15.1.0', required: false)
if gvc.found()
  message('graphviz is enabled')
  add_project_arguments('-DHYPHA_GRAPHVIZ_ENABLED', language: [ 'c', 'cpp' ])
  all_hypha_deps += gvc
endif

wamr = dependency('libiwasm', version: '>=2.2.0', required: false) 
if wamr.found()
  message('wamr is enabled')
  add_project_arguments('-DHYPHA_WAMR_ENABLED', language: [ 'c', 'cpp' ])
  all_hypha_deps += wamr
endif

tracy_dep = dependency('tracy', required: false)
if enable_profiling
  if tracy_dep.found()
    message('profiling is enabled with tracy')
    all_hypha_deps += tracy_dep
    add_project_arguments('-DHYPHA_ENABLE_PROFILING', language: [ 'c', 'cpp' ])
  endif
endif

gtest_dep = dependency('gtest', main: true, required: false)

subdir('libs/')
subdir('test/')

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
    hypha_inc_dir,
  ],
)
