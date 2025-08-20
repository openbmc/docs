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

### `misc-use-internal-linkage`

It is suspected that this requires a non-trivial amount of work to enable and
requires experimentation.

### `modernize-pass-by-value`

This requires experimentation to determine the amount of work necessary to
enable this.

### `modernize-use-designated-initializers`

This requires experimentation to determine the amount of work necessary to
enable this.

### `modernize-use-nodiscard`

This requires experimentation to determine the amount of work necessary to
enable this.

### `modernize-use-trailing-return-type`

This requires experimentation to determine the amount of work necessary to
enable this.

We may want to enhance `clang-tidy` first to add an option allowing short
returns like `bool`.

## Highly discouraged checks

The following checks and settings are ones that the project does not want to
enable. Each check (or check-group) has an explanation as to why it is
considered inappropriate for OpenBMC.

### `abseil-*`

We do not use abseil.

### `altera-*`

These are intended for OpenCL only.

### `android-*`

These are Android specific checks.

### `boost-use-ranges`

Since we are using C++26 (or later) we should prefer `std::ranges` over Boost
ranges anyhow.

### `bugprone-easily-swappable-parameters`

This is not suggested by the C++ Core Guidelines and is a very strict check that
would require significant refactoring across the project.

### `bugprone-no-escape`

This is an Apple specific check.

### `bugprone-nondeterministic-pointer-iteration-order`

These are not certain to be an error and per `clang-tidy` is only advisory.

### `cert-*`

The majority of `cert-*` warnings are aliases and should not be enabled.

The following are non-aliases which are to be enabled:

```text
    cert-dcl50-cpp,
    cert-dcl58-cpp,
    cert-env33-c,
    cert-err34-c,
    cert-err52-cpp,
    cert-err58-cpp,
    cert-err60-cpp,
    cert-flp30-c,
    cert-mem57-cpp,
    cert-msc50-cpp,
    cert-msc51-cpp,
    cert-oop57-cpp,
    cert-oop58-cpp,
```

### `cppcoreguidelines-avoid-c-arrays`

This is an alias to `modernize-avoid-c-arrays`.

### `cppcoreguidelines-avoid-magic-numbers`

This is an alias to `readability-avoid-magic-numbers`.

### `cppcoreguidelines-c-copy-assignment-signature`

This is an alias to `misc-unconventional-assign-operator`.

### `cppcoreguidelines-explicit-virtual-functions`

This is an alias to `modernize-use-override`.

### `cppcoreguidelines-macro-to-enum`

This is an alias to `modernize-macro-to-enum`.

### `cppcoreguidelines-narrowing-conversions`

This is an alias to `bugprone-narrowing-conversions`.

### `cppcoreguidelines-noexcept-destructor`

This is an alias to `performance-noexcept-destructor`.

### `cppcoreguidelines-noexcept-move-operations`

This is an alias to `performance-noexcept-move-constructor`.

### `cppcoreguidelines-noexcept-swap`

This is an alias to `performance-noexcept-swap`.

### `cppcoreguidelines-non-private-member-variables-in-classes`

This is an alias to `misc-non-private-member-variables-in-class`.

### `cppcoreguidelines-owning-memory`

We do not have the "Guidelines Support Library" in use.

### `cppcoreguidelines-special-member-functions`

This is an alias to `modernize-use-default-member-init`.

### `darwin-*`

These are Darwin (MacOS) specific checks.

### `fuchsia-*`

These are Fuchsia specific checks.

### `google-*`

Some of these are Google specific checks that are not relevant to OpenBMC. Some
of these are aliases.

The following are enabled:

```text
    google-build-namespaces,
    google-build-using-namespace,
    google-default-arguments,
    google-explicit-constructor,
    google-global-names-in-headers,
```

### `hicpp-*`

Most `hicpp-*` checks are aliases.

The following are enabled:

```text
    hicpp-exception-baseclass,
    hicpp-no-assembler,
```

### `linuxkernel-must-check-errs`

This is a Linux Kernel specific check.

### `llvm-*`

Most of these are only relevant for LLVM coding.

The following are enabled:

```text
    llvm-namespace-comment,
```

### `misc-no-recursion`

We do not currently limit recursion.

### `misc-use-anonymous-namespace`

The use of `static` is pervasive and this check provides no strong technical
basis for favoring anonymous namespace over static.

### `modernize-replace-disallow-copy-and-assign-macro`

We have no such macro in use in OpenBMC, so this check is unnecessary.

### `modernize-replace-random-shuffle`

We have long-ago upgraded from C++17, which is where `random_shuffle` was
removed. This check is unnecessary.

### `modernize-use-std-format`

This is related to Abseil and unnecessary.

### `mpi-*`

These are MPI specific checks.

### `objc-*`

These are ObjectiveC specific checks.

### `openmp-*`

These are OpenMP specific checks.

### `performance-enum-size`

This decreases readability for a minor performance improvement.

### `portability-restrict-system-includes`

OpenBMC does not need portability on system includes as we allow many system
includes via Meson/Bitbake dependencies.

### `portability-simd-intrinsics`

OpenBMC does not use SIMD.

### `portability-template-virtual-member-function`

OpenBMC does not support MSVC.

### `readability-identifier-length`

OpenBMC does not currently have standards on identifier lengths.

### `readability-identifier-naming`

OpenBMC does have standards for identifier naming but allows flexibility that
cannot be specified with `clang-tidy`.

### `readability-operators-representation`

OpenBMC does not have conventions for traditional tokens vs alternatives (such
as `&&` vs `and`). The default `clang-tidy` option also enforces no convention,
unless extra options are configured. Therefore, since we specify no options and
have no conventions, this check performs no action.

### `readability-qualified-auto`

OpenBMC does not have this convention.

### `zircon-*`

These are Zircon specific checks.
