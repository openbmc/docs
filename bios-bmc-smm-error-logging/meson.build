project(
  'bejdecoder',
  'c',
  'cpp',
  version: '0.1',
  meson_version: '>=0.57.0',
  default_options: [
    'cpp_std=c++20',
    'werror=true',
    'warning_level=3',
  ])
bejdecoder_incs = include_directories('include')
subdir('src')
if not get_option('tests').disabled()
  subdir('test')
endif

# installation of systemd service files
# subdir('service_files')