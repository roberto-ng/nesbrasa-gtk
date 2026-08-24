api_version = '0.1'

nesbrasa_sources = [
    'cores.cpp',
    'controle.cpp',
    'cpu.cpp',
    'instrucao.cpp',
    'memoria.cpp',
    'nesbrasa.cpp',
    'ppu.cpp',
    'util.cpp',
    'mapeadores/cartucho.cpp',
    'mapeadores/nrom.cpp',
]

nesbrasa_headers = [
  'cores.hpp',
  'controle.hpp',
  'cpu.hpp',
  'instrucao.hpp',
  'memoria.hpp',
  'nesbrasa.hpp',
  'ppu.hpp',
  'util.hpp',
  'tipos_numeros.hpp',
  'mapeadores/cartucho.hpp',
  'mapeadores/nrom.hpp',
]


nesbrasa_deps = [

]

install_headers(nesbrasa_headers, subdir: 'nesbrasa')

nesbrasa_lib = shared_library('nesbrasa' + api_version,
  nesbrasa_sources,
  include_directories: [inc, inc_mapeadores],
  dependencies: nesbrasa_deps,
  install: true,
)

nesbrasa_dep = declare_dependency(include_directories: [inc, inc_mapeadores], link_with: nesbrasa_lib)

pkg = import('pkgconfig')

pkg.generate(
  description: 'Um emulador de NES',
    libraries: nesbrasa_lib,
         name: 'braza-nes',
     filebase: 'nesbrasa' + api_version,
      version: meson.project_version(),
      subdirs: 'nesbrasa',
  install_dir: join_paths(get_option('libdir'), 'pkgconfig')
)
