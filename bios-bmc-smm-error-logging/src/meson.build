libbejdecoder_lib = static_library(
  'bejdecoder',
  'bej_decoder.cpp',
  'bej_decoder_core.c',
  'rde_common.c',
  'bej_dictionary.c',
  include_directories : bejdecoder_incs,
  implicit_include_directories: false,
)

libbejdecoder = declare_dependency(
  include_directories: bejdecoder_incs,
  link_with: libbejdecoder_lib
)
