project(
	'nesbrasa',
	['cpp', 'c'],
	version: '0.1.0',
	meson_version: '>= 0.46.0',
	default_options : ['c_std=c11', 'cpp_std=c++17'],
)

i18n = import('i18n')

subdir('core')
subdir('frontends/gtk')

if host_machine.system() == 'linux'
    meson.add_install_script('frontends/gtk/build-aux/meson/postinstall.py')
endif
