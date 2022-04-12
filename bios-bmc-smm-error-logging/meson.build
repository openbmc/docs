project(
  'bmc-bios-buffer',
  'cpp',
  version: '0.1',
  meson_version: '>=0.57.0',
  default_options: [
    'cpp_std=c++20',
    'warning_level=3',
    'werror=true',
  ])

root_inc = include_directories('.')

sys_lib = static_library(
  'sys',
  'internal/sys.cpp',
  implicit_include_directories: false)

sys_dep = declare_dependency(
  link_with: sys_lib)

bmcbiosbuffer_inc = include_directories('include')
subdir('src')

# installation of systemd service files
subdir('service_files')