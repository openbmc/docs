gtest = dependency('gtest', main: true, disabler: true, required: get_option('tests'))
gmock = dependency('gmock', disabler: true, required: get_option('tests'))
nlohmann_json = dependency('nlohmann_json', disabler: true, required: get_option('tests'))

if not gtest.found() or not gmock.found()
  gtest_proj = import('cmake').subproject('googletest', required: false)
  if gtest_proj.found()
    gtest = declare_dependency(
      dependencies: [
        gtest_proj.dependency('gtest'),
        gtest_proj.dependency('gtest_main'),
      ])
    gmock = gtest_proj.dependency('gmock')
  else
    assert(not get_option('tests').enabled(), 'Googletest is required')
  endif
endif

if not nlohmann_json.found()
  nlohmann_json_proj = import('cmake').subproject('nlohmann', required: false)
  if nlohmann_json_proj.found()
    nlohmann_json = gtest_proj.dependency('nlohmann_json')
  else
    assert(not get_option('tests').enabled(), 'nlohmann_json is required')
  endif
endif

bejdecoder_test_incs = include_directories('include')

gtests = [
  'bej_decoder',
]
foreach t : gtests
  test(t, executable(t.underscorify(), t + '_test.cpp',
                     build_by_default: false,
                     include_directories : bejdecoder_test_incs,
                     implicit_include_directories: false,
                     dependencies: [libbejdecoder, gtest, gmock]))
endforeach
