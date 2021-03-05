project(
    'template-application',
    'cpp',
    default_options: [
        'warning_level=3',
        'werror=true',
        'cpp_std=c++17',
        'cpp_rtti=false',
    ],
    license: 'Apache-2.0',
    version: '0.1',
)

systemd = dependency('systemd')
sdbusplus = dependency('sdbusplus')

cxx = meson.get_compiler('cpp')
add_project_arguments(
cxx.get_supported_arguments([
'-DBOOST_ASIO_USE_TS_EXECUTOR_AS_DEFAULT',
'-DBOOST_ASIO_DISABLE_THREADS',
'-DBOOST_NO_RTTI',
'-DBOOST_NO_TYPEID',
'-DBOOST_ALLOW_DEPRECATED_HEADERS'
]),
language : 'cpp')

systemd_system_unit_dir = systemd.get_pkgconfig_variable(
    'systemdsystemunitdir',
    define_variable: ['prefix', get_option('prefix')])

conf_data = configuration_data()
conf_data.set('MESON_INSTALL_PREFIX', get_option('prefix'))

configure_file(input: 'xyz.openbmc_project.templateapp.service.in',
               output: 'xyz.openbmc_project.templateapp.service',
               install_dir: systemd_system_unit_dir,
               configuration: conf_data,
               install: true)

template_app_a = static_library(
    'template_app_a',
    'src/TemplateApp.cpp',
    include_directories: 'include',
)

executable(
    'template-app',
    'src/TemplateAppMain.cpp',
    dependencies: [
      systemd,
      sdbusplus,
    ],
    include_directories: 'include',
    install: true,
    link_with: [
	    template_app_a,
    ],
)

if get_option('tests').enabled()
  gtest = dependency('gtest', main: true)
  gmock = dependency('gmock')
  subdir('test')
endif