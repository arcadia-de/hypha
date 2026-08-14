project(
  'hypha',
  [ 'c', 'cpp' ],
  version: '1.0',
  default_options: ['c_std=c23'],
  meson_version: '>= 1.10.0',
)
add_project_arguments('-D_XOPEN_SOURCE=700', language: 'c')

if get_option('buildtype') == 'debug'
  message('debug mode is enabled')
  add_project_arguments('-DHYPHA_DEBUG', language: [ 'c', 'cpp' ])
endif

threads = dependency('threads', required: true)
libuv = dependency('libuv', version: '>=1.0.0', required: true)
lua54 = dependency('lua54', version: '>=5.4.8', required: true)
jansson = dependency('jansson', version: '>=2.15.0', required: true)
archive = dependency('libarchive', version: '>= 3.8.8', required: false)
git2 = dependency('libgit2', version: '>= 1.9.4', required: false)

gvc = dependency('libgvc', version: '>= 15.1.0', required: false)
if gvc.found()
  message('graphviz is enabled')
  add_project_arguments('-DHYPHA_GRAPHVIZ_ENABLED', language: [ 'c', 'cpp' ])
endif

wamr = dependency('libiwasm', version: '>=2.2.0', required: false) 
if wamr.found()
  message('wamr is enabled')
  add_project_arguments('-DHYPHA_WAMR_ENABLED', language: [ 'c', 'cpp' ])
endif

gtest_dep = dependency('gtest', main: true, required: false)

subdir('libs/')

cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required: false)
pkg = import('pkgconfig')

all_hypha_deps = [
  threads,
  libuv,
  lua54,
  jansson,
  archive,
  git2,
]

if gvc.found()
  all_hypha_deps += gvc
endif

if wamr.found()
  all_hypha_deps += wamr
endif

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
    hypha_api_inc_dir,
    hypha_inc_dir,
    hypha_resource_graph_inc_dir,
  ],
)
