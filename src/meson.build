nesbrasa_gtk_sources = [
  'main.cpp',
  'janela_principal.cpp',
]

nesbrasa_gtk_deps = [
  dependency('gtkmm-3.0', version: '>= 3.18'),
]

gnome = import('gnome')

nesbrasa_gtk_sources += gnome.compile_resources('nesbrasa-gtk-resources',
  'nesbrasa-gtk.gresource.xml',
  c_name: 'nesbrasa_gtk'
)

executable('nesbrasa-gtk', nesbrasa_gtk_sources,
  dependencies: nesbrasa_gtk_deps,
  install: true,
)
