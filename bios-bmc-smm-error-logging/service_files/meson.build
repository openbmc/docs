systemd = dependency('systemd')
conf_data = configuration_data()
conf_data.set('BINDIR', get_option('prefix') / get_option('bindir'))
configure_file(
  input: 'com.google.gbmc.bmcbiosbuffer.service.in',
  output: 'com.google.gbmc.bmcbiosbuffer.service',
  configuration: conf_data,
  install: true,
  install_dir: systemd.get_variable(pkgconfig: 'systemdsystemunitdir'))
