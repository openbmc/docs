project(
    'template-application',
    'cpp',
    default_options: [
        'warning_level=3',
        'werror=true',
        'cpp_std=c++17',
        # note: This is allowed in certain contexts, but provides binary size
        # improvements for applications that don't require rtti
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
    'systemdsystemunitdir'
)

configure_file(input: 'xyz.openbmc_project.templateapp.service',
               output: 'xyz.openbmc_project.templateapp.service',
               install_dir: systemd_system_unit_dir,
               copy: true,
               install: true)

template_app_a = static_library(
    'template_app_a',
    'src/template_app.cpp',
    include_directories: 'include',
)

executable(
    'template-app',
    'src/template_app_main.cpp',
    dependencies: [
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
  template_app_test = executable(
      'template-app-test',
      'test/template_app_test.cpp',
      dependencies: [
          sdbusplus,
          gmock,
          gtest,
      ],
      include_directories: [
          'include',
      ],
      link_with: [
          template_app_a,
      ],
  )

  test('test_template_app', template_app_test)
endif
