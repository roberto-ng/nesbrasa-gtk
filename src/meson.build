nesbrasa_gtk_sources = [
  'arquivo.cpp',
  'main.cpp',
  'janela_principal.cpp',
]

nesbrasa_gtk_deps = [
  dependency('gtkmm-3.0', version: '>= 3.18'),
]

gnome = import('gnome')

ldflags = []

if host_machine.system() == 'windows'
  cc = meson.get_compiler('c')
  ldflags = cc.get_supported_link_arguments('-Wl,-subsystem,windows')
endif

nesbrasa_gtk_sources += gnome.compile_resources('nesbrasa-gtk-resources',
  'nesbrasa-gtk.gresource.xml',
  c_name: 'nesbrasa_gtk'
)

nesbrasa_dep = nesbrasa_proj.get_variable('nesbrasa_dep')

executable('nesbrasa-gtk', nesbrasa_gtk_sources,
  dependencies: [nesbrasa_dep, nesbrasa_gtk_deps],
  link_args: ldflags,
  install: true,
)
