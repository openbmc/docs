# C++ Coding Style and Conventions

## General Philosophy

Being an extensive and complicated language, there are often differences of
opinions on "good" and "bad" C++ code. Bjarne Stroustrup has said "Within C++ is
a smaller, simpler, safer language struggling to get out." We are striving to
write in this variant of C++ and are therefore following the "C++ Core
Guidelines" that Bjarne and Herb Sutter introduced at CppCon 2015.

Beyond a set of rules that help codify "good" and "bad" C++, we have general
principles that help us align the software we develop with the constraints
within the problem domain being solved by OpenBMC. These are:

1.  Code should be clear and concise.
2.  Code should be written with modern practices.
3.  Code should be performant.

### Code should be clear and concise

> Brevity is the soul of wit.

It is important that code be optimized for the reviewer and maintainer and not
for the writer. Solutions should avoid tricks that detract from the clarity of
reviewing and understanding it.

Modern practices allow C++ to be an expressive, but concise, language. We tend
to favor solutions which succinctly represent the problem in as few lines as
possible.

When there is a conflict between clarity and conciseness, clarity should win
out.

### Code should be written with modern practices

We strive to keep our code conforming to and utilizing of the latest in C++
standards. Today, that means all C++ code should be compiled using C++20
compiler settings. As the C++23 standard is finalized and compilers support it,
we will move to it as well.

We also strive to keep the codebase up-to-date with the latest recommended
practices by the language designers. This is reflected by the choice in
following the C++ Core Guidelines.

We finally desire to have computers do our thinking for us wherever possible.
This means having Continuous Integration tests on each repository so that
regressions are quickly identified prior to merge. It also means having as much
of this document enforced by tools as possible by, for example, clang-format and
clang-tidy.

For those coming to the project from pre-C++11 environments we strongly
recommend the book "Effective Modern C++" as a way to get up to speed on the
differences between C++98/03 and C++11/14/17/20.

### Code should be performant.

OpenBMC targets embedded processors that typically have 32-64MB of flash and
similar processing power of a typical smart-watch available in 2016. This means
that there are times where we must limit library selection and/or coding
techniques to compensate for this constraint. Due to the current technology,
performance evaluation is done in order of { code size, cpu utilization, and
memory size }.

From a macro-optimization perspective, we expect all solutions to have an
appropriate algorithmic complexity for the problem at hand. Therefore, an
`O(n^3)` algorithm may be rejected even though it has good clarity when an
`O(n*lg(n))` solution exists.

## Global Guidelines and Practices

Please follow the guidelines established by the C++ Core Guidelines (CCG).

https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md

[[Last reviewed revision is 53bc78f]]

Exceptions:

### Guideline Support Library (GSL)

We do not currently utilize the Guideline Support Library provided by the CCG.
Any recommendation within the CCG of GSL conventions may be ignored at this
time.

### Style recommendations

The following are not followed:

- NL.10 Avoid CamelCase
- NL.17 Use K&R-derived layout

## Library and Feature Specifics

Additional recommendations within the OpenBMC project on specific language
features or libraries.

### Exceptions

We do use exceptions as a basis for error handling within OpenBMC.

### Boost

Use of boost is allowed, under the following circumstances:

1. Use is done as a header-only library. This allows unused functions and
   methods to be removed by the compiler at link time, and avoids adding large
   amounts of overhead and flash usage.
2. Implementers should include the lowest level header required to solve the
   problem at hand. This allows uses to be found and moved forward when new
   standards are available, as well as reduce compile times, and decrease the
   possibility of accidental use. (ie, #include <boost/container/flat_map.hpp>
   not #include <boost/container.hpp>)
3. The module used should not have an equivalent in the std namespace that meets
   the same requirements for implementation or clarity: For example, std::chrono
   should be preferred over boost::chrono. std::array over boost::array.
4. Use does not conflict with any of the core tenants of this coding standard
   (clarity, modern practices, or performance).

### iostream

The iostream conventions of using 'operator<<' contribute to an increased code
size over printf-style operations, due to individual function calls for each
appended value. We therefore do not use iostreams, or iostream-style APIs, for
logging.

There are cases when using an iostream utility (such as sstream) can result in
clearer and similar-sized code. iostream may be used in those situations.

## Coding Style

Indentation, naming practices, etc.

### General

- Line length should be limited to 80 characters.
- Indentation should be done with 4 space characters.
- Files should use Unix-style newlines (\n).

### Clang Formatting

Individual OpenBMC repositories can use
[clang-format](https://clang.llvm.org/docs/ClangFormat.html) if desired. The
OpenBMC CI infrastructure will automatically verify the code formatting on code
check-in if a .clang_format file is found within the root directory of the
repository. This allows for automatic validation of code formatting upon
check-in.

If a custom configuration is desired, such as using different clang formatting
for C and C++ files, a format-code.sh script can be created, which can for
example use different .clang\* files as input depending on the file type. The
format-code.sh script will be executed as part of CI if found in the root
directory of the repository, and will check that there are no files that were
modified after running it (same check as running clang).

OpenBMC requires a clang-format of version 6.0 or greater. An example of how to
run clang-format against all code in your repo can be found by referencing the
[tool](https://github.com/openbmc/openbmc-build-scripts/blob/master/scripts/format-code.sh)
used by CI.

[Example .clang-format](https://www.github.com/openbmc/docs/blob/master/style/cpp/.clang-format)

### Bracket style

- Utilize 'Allman' style brackets. Brackets are on their own line at the same
  indentation level as the statement that creates the scope.

```
if (condition)
{
    ...
}
```

```
void foo()
{
    ...
}
```

- Even one line conditional and loop statements should have brackets.

```
/// Wrong.
if (condition)
    do_something;

/// Correct
if (condition)
{
    do_something;
}
```

### Indentation

- Content within a namespace should be at the same indentation level as the
  namespace itself.

```
namespace foo
{

content

}
```

- Content within a class / struct should be indented.

```
class Foo
{
    public:
        Foo();
}
```

- Content within a function / conditional / loop should be indented.

```
void foo()
{
    while (1)
    {
        if (bar())
        {
            ...
        }
    }
}
```

- Switch / case statements should be indented.

```
switch (foo)
{
    case bar:
    {
        bar();
        break;
    }

    case baz:
    {
        baz();
        break;
    }
}
```

- Labels should be indented so they appear at 1 level less than the current
  indentation, rather than flush to the left. (This is not to say that goto and
  labels are preferred or should be regularly used, but simply when they are
  used, this is how they are to be used.)

```
void foo()
{
    if (bar)
    {
        do
        {
            if (baz)
            {
                goto exit;
            }

        } while(1);

    exit:
        cleanup();
    }
}
```

### Naming Conventions.

- We generally abstain from any prefix or suffix on names.
- Acronyms should be same-case throughout and follow the requirements as in
  their appropriate section.

```
/// Correct.
SomeBMCType someBMCVariable = bmcFunction();

/// Wrong: type and variable are mixed-case, function isn't lowerCamelCase.
SomeBmcType someBmcVariable = BMCFunction();
```

### Header Ordering

Header inclusion order for a header file:

```
local headers (e.g. "daemon_sys.hpp")
c-libraries
cpp-libraries (including openbmc libraries)
```

Header inclusion order for a source file:

```
source.hpp (if applicable)
local headers
c-libraries
cpp-libraries
```

All in alphabetically sorted order.

#### Files

- C++ headers should end in ".hpp". C headers should end in ".h".
- C++ files should be named with lower_snake_case.

#### Types

- Prefer 'using' over 'typedef' for type aliases.
- Structs, classes, enums, and typed template parameters should all be in
  UpperCamelCase.
- Prefer namespace scoping rather than long names with prefixes.
- A single-word type alias within a struct / class may be lowercase to match STL
  conventions (`using type = T`) while a multi-word type alias should be
  UpperCamelCase (`using ArrayOfT = std::array<T, N>`).
- Exception: A library API may use lower_snake_case to match conventions of the
  STL or an underlying C library it is abstracting. Application APIs should all
  be UpperCamelCase.
- Exception: A for-convenience template type alias of a template class may end
  in `_t` to match the conventions of the STL.

```
template <typename T>
class Foo
{
    using type = std::decay_t<T>;
};

template <typename T> using foo_t = Foo<T>::type;
```

#### Variables

- Variables should all be lowerCamelCase, including class members, with no
  underscores.

#### Functions

- Functions should all be lowerCamelCase.
- Exception: A library API may use lower_snake-case to match conventions of the
  STL or an underlying C library it is abstracting. Application APIs should all
  be lowerCamelCase.

#### Constants

- Constants and enum members should be named like variables in lowerCamelCase.

#### Namespaces

- Namespaces should be lower_snake_case.
- Top-level namespace should be named based on the containing repository.
- Favor a namespace called 'details' or 'internal' to indicate the equivalent of
  a "private" namespace in a header file and anonymous namespaces in a C++ file.

### Header Guards

Prefer '#pragma once' header guard over '#ifndef'-style.

### Additional Whitespace

- Follow NL.18: Use C++-style declarator layout.

```
foo(T& bar, const S* baz); /// Correct.
foo(T &bar, const S *baz); /// Incorrect.
```

- Follow NL.15: Use spaces sparingly.

- Insert whitespace after a conditional and before parens.

```
if (...)
while (...)
for (...)
```

- Insert whitespace around binary operators for readability.

```
foo((a-1)/b,c-2); /// Incorrect.
foo((a - 1) / b, c - 2); /// Correct.
```

- Do not insert whitespace around unary operators.

```
a = * b;  /// Incorrect.
a = & b;  /// Incorrect.
a = b -> c;  /// Incorrect.
if (! a)  /// Incorrect.
```

- Do not insert whitespace inside parens or between a function call and
  parameters.

```
foo(x, y); /// Correct.
foo ( x , y ); /// Incorrect.

do (...)
{
} while(0); /// 'while' here is structured like a function call.
```

- Prefer line-breaks after operators to show continuation.

```
if (this1 == that1 &&
    this2 == that2) /// Correct.

if (this1 == that1
    && this2 == that2) /// Incorrect.
```

- Long lines should have continuation start at the same level as the parens or
  all all items inside the parens should be at a 2-level indent.

```
reallyLongFunctionCall(foo,
                       bar,
                       baz); // Correct.

reallyLongFunctionCall(
        foo,
        bar,
        baz); // Also correct.

reallyLongFunctionCall(
        foo, bar, baz); // Similarly correct.

reallyLongFunctionCall(foo,
        bar,
        baz); // Incorrect.
```

### Misc Guidelines.

- Always use `size_t` or `ssize_t` for things that are sizes, counts, etc. You
  need a strong rationale for using a sized type (ex. `uint8_t`) when a size_t
  will do.

- Use `uint8_t`, `int16_t`, `uint32_t`, `int64_t`, etc. for types where size is
  important due to hardware interaction. Do not use them, without good reason,
  when hardware interaction is not involved; prefer size_t or int instead.
