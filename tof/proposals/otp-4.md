---
OTP: 4
Title: converge on meson-build
Author: Patrick Williams <patrick@stwcx.xyz>
Created: 2021-10-11
Requires: N/A
Replaces: N/A
Superseded-By: N/A
---

# OTP-4: Converge on Meson Build

## Background

Historically, many repositories were originally written using the autotools
build system, some were added later using CMake, and lately many have been using
(or converted to) [meson-build][1].  As of [2021-10-11][2] there are 33
repositories using meson, 20 using autotools, and 7 using cmake as referenced by
recipes in the meta-phosphor layer.  This has lead to a fragmentation of
tooling, expertise, and best practices.

## Actions

* All new repositories in the openbmc organization must support meson.
    * Any corresponding recipe in [openbmc/openbmc][3] shall use the meson build
      process.
    * At the discretion of the repository maintainer(s) additional build systems
      may be supported.  This is intended to mainly be used in repositories
      which might benefit from wider adoption outside OpenBMC by utilizing
      additional build systems.

* Any existing repository using autotools must be updated to use meson by
  2022-03-01 or the recipe will be deleted.

* Any existing repository using cmake must be updated to use meson by 2022-08-01
  or the recipe will be deleted.

## Rationale

Autotools is antiquated and cumbersome and its continued use is impacting
developer productivity.  Autotools support leverages [`autoconf-archive`][4] to
detect available C++ standards but C++20 is still [not supported][5] over 1 year
after the standard was voted for acceptance.  There is also very limited support
for building these packages outside of an OE-SDK environment.

CMake is a more modern build system but is similarly lacking a widely applied
method for building outside of an OE-SDK environment.

Meson is both modern and easily adapted to utilize [meson subproject][6] wrap
files which allow obtaining and building all the necessary dependencies to build
and test a repository on a typical Linux development system.  Meson also has
built-in support for features such as unit testing, code coverage, code
formatting, and static analysis.

Converging on meson will:
    * Reduce the necessity of maintaining tooling for building the 3 different
      systems in CI.
    * Consolidate build system expertise into a single system and approach.
    * Improve developer productivity by enabling development activities outside
      of Yocto and OE-SDK environments.
    * Enable better alignment on and adoption of build system best practices.
    * Enable rapid adaptation of newer C++ standards.

---

[1]: https://github.com/mesonbuild/meson
[2]: https://gist.github.com/williamspatrick/b5fd618ef597cd6ad6dd97a633d5b264
[3]: https://github.com/openbmc/openbmc
[4]: https://github.com/autoconf-archive/autoconf-archive
[5]: https://github.com/autoconf-archive/autoconf-archive/blob/e68e8f6f62aea14b1ce980e75442d62abd668a95/m4/ax_cxx_compile_stdcxx.m4#L53
[6]: https://www.stwcx.xyz/blog/2021/04/18/meson-subprojects.html
