template_app_test = executable(
    'template-app-test',
    'TemplateAppTest.cpp',
    dependencies: [
      systemd,
      sdbusplus,
      gmock,
      gtest,
    ],
    include_directories: [
        '../include',
    ],
    link_with: [
	    template_app_a,
    ],
)

test('test_template_app', template_app_test)