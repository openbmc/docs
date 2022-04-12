bmcbiosbuffer_pre = declare_dependency(
  include_directories: [root_inc, bmcbiosbuffer_inc],
  dependencies: [
    dependency('threads'),
    sys_dep,
])

bmcbiosbuffer_lib = static_library(
  'bmcbiosbuffer',
  'pci_handler.cpp',
  implicit_include_directories: false,
  dependencies: bmcbiosbuffer_pre)

bmcbiosbuffer_dep = declare_dependency(
  link_with: bmcbiosbuffer_lib,
  dependencies: bmcbiosbuffer_pre)

executable(
  'bmcbiosbuffer',
  'main.cpp',
  implicit_include_directories: false,
  dependencies: bmcbiosbuffer_dep,
  install: true,
)
