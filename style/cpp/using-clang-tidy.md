# Using Clang-tidy

## Overview

There is a reference `.clang-tidy` available here, which maintainers should use
as the encouraged set of `clang-tidy` checks and options. In order to minimize
the burden on updating Clang to later versions and the potential frustration of
disparate checks and styles between repositories, maintainers should only allow
enabling of options already documented. If a new `clang-tidy` check is useful, a
change to the reference is sufficient and should not have much friction.

In addition to the `clang-tidy` checks already enabled, we have found two
additional types of checks:

- Future enabled checks.
  - These are seen as useful checks for improving code quality but may require
    significant engineering effort to roll out as there are likely many
    violations. These checks can be promoted to the reference when they have
    demonstrated sufficient enabling (in multiple repositories).
- Highly discouraged checks.
  - There are some checks which are not useful to OpenBMC (often related to APIs
    not used in the project) or are in opposition to our readily accepted style.
  - Some checks have aliases that are not as obviously named as the alternative
    and should only be enabled via the primary name.

## Future enabled checks

The following checks and settings are ones that the project would like to
enable, but has had trouble either in engineering or has other higher priority
rework. There is general agreement that the below checks _should_ be useful, but
have not concretely enabled them across the project.

### `readability-function-size` and `readability-function-cognitive-complexity`

```yaml
Checks: "-*, readability-function-size,
  readability-function-cognitive-complexity
  "

CheckOptions:
  - key: readability-function-size.LineThreshold
    value: 60 # [1]
  - key: readability-function-size.ParameterThreshold
    value: 6 # [2]
  - key: readability-function-cognitive-complexity.Threshold
    value: 25 # [3]

# [1] https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#f3-keep-functions-short-and-simple
# [2] https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#f2-a-function-should-perform-a-single-logical-operation
# [3] https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#f3-keep-functions-short-and-simple
# However cognitive complexity != cyclomatic complexity. Therefore using the clang-tidy default value,
# as cyclomatic complexity seems to not be implemented in clang-tidy.

# [1],[2],[3] do not have to be enforced or applied project-wide yet.
```

## Highly discouraged checks

The following checks and settings are ones that the project does not want to
enable. Each check (or check-group) has an explanation as to why it is
considered inappropriate for OpenBMC.
