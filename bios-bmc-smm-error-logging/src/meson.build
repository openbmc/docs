bmcbiosbuffer_pre = declare_dependency(
  dependencies: [
    dependency('threads'),
    sys_dep,
])

bmcbiosbuffer_dep = declare_dependency(
  dependencies: bmcbiosbuffer_pre,)

executable(
  'bmcbiosbuffer',
  'main.cpp',
  implicit_include_directories: false,
  dependencies: bmcbiosbuffer_dep,
  install: true,
)
