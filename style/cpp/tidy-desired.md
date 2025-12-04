# Desired clang-tidy checks

The following file contains checks and settings that the project would like to
enable, but has had trouble either in engineering or has other higher priority
rework. There is general agreement that the below checks _should_ be useful, but
have not concretely enabled them across the project.

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
