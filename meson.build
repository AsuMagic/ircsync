project('ircsync', 'cpp',
	version: '0.0.1-alpha',
	license: 'TODO',
	meson_version : '>=0.48.0',
	default_options: [
		'cpp_std=c++17',
		'warning_level=3',
		'werror=true',
		'buildtype=debug'
  ],
)

add_project_arguments(
	'-Wall',
	'-Wno-unused-parameter',
	'-Wextra',
	'-pthread',
	language: 'cpp'
)

include_folder = include_directories('.')

ircsync_src = files(
	'main.cpp',
	'irc.cpp',
)

cxx = meson.get_compiler('cpp')

lib_fmt = cxx.find_library('fmt',
	dirs: [
		'/usr/lib',
		'/usr/lib64',
	]
)

include_fmt = include_directories('/usr/include')

boost = dependency('boost',
	modules: ['system']
)
pthread = dependency('threads')

ircsync_deps = [
	boost,
	pthread,
]

executable(
	'ircsync',
	ircsync_src,
	include_directories: [
		include_folder,
		include_fmt,
	],
	dependencies: [
		ircsync_deps,
		lib_fmt
	],
)
