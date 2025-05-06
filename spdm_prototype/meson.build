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

libspdm_dep = dependency('libspdm', required: true)

if not libspdm_dep.found()
  error('libspdm not found . Please ensure libspdm is installed.')
endif

spdmd_deps = [
    dependency('phosphor-logging'),
    dependency('sdbusplus'),
    dependency('phosphor-dbus-interfaces'),
    libspdm_dep,
]

spdmd_src = [
    'spdmd/spdmd.cpp',
    'spdmd/spdmd_app.cpp',
    'spdmd/spdm_discovery.cpp',
    'spdmd/mctp_discovery.cpp',
    'spdmd/spdm_dbus_responder.cpp',
    'spdmd/spdm_device_secret_lib_stub.cpp',
    'spdmd/component_integrity_dbus.cpp',
    'spdmd/trusted_component_dbus.cpp',
    'spdmd/libspdm_mctp_transport.cpp',
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
