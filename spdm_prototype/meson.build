project('spdmcpp',
	'cpp', 'c',
	version: '0.1.0',
	meson_version: '>=0.58.0',
	default_options: [
		'cpp_std=c++20',
		'warning_level=3',
		'buildtype=debugoptimized'
	]
)

# Add optimization flag
add_project_arguments('-O2', language: 'cpp')

spdmd_deps = [
    dependency('phosphor-logging'),
    dependency('sdbusplus'),
]

spdmd_src = [
    'spdmd/spdmd.cpp',
    'spdmd/spdmd_app.cpp',
]

spdmd_includes = include_directories('spdmd')

executable(
    'spdmd',
    spdmd_src,
    dependencies: spdmd_deps,
    include_directories: spdmd_includes,
    install: true,
    install_dir: get_option('bindir')
)
