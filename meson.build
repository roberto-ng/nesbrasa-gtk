project(
	'nesbrasa-gtk',
	['cpp', 'c'],
	version: '0.1.0',
	meson_version: '>= 0.46.0',
	default_options : ['c_std=c11', 'cpp_std=c++17'],
)

nesbrasa_proj = subproject('nesbrasa')

i18n = import('i18n')

subdir('data')
subdir('src')
subdir('po')

if host_machine.system() == 'linux'
    meson.add_install_script('build-aux/meson/postinstall.py')
endif
