---
OTP: 4
Title: converge on meson-build
Author: Patrick Williams <patrick@stwcx.xyz>
Created: 2022-04-14
Type: Guideline
Requires: N/A
Replaces: N/A
Superseded-By: N/A
---

# OTP-4: Converge on Meson Build

## Background

Historically, many repositories were originally written using the autotools
build system, some were added later using CMake, and lately many have been using
(or converted to) [meson-build][1]. As of [2022-04-14][2] there are 39
repositories using meson, 17 using autotools, and 5 using CMake as referenced by
recipes in the meta-phosphor layer. This has lead to a fragmentation of tooling,
expertise, and best practices.

## Actions

- All new repositories in the openbmc organization must support meson.

  - Any corresponding recipe in [openbmc/openbmc][3] shall use the meson build
    process.
  - At the discretion of the repository maintainer(s) additional build systems
    may be supported. This is intended to mainly be used in repositories which
    might benefit from wider adoption outside OpenBMC by utilizing additional
    build systems.
  - With a future proposal, and the approval of the TOF, additional build
    systems may be experimented with in the future as new build systems come out
    within the development community.

- Establish the following timelines for full deprecation of autotools and CMake:
  - Any existing repository using autotools should be updated to use meson by
    2022-11-01.
  - Any existing repository using CMake should be updated to use meson by
    2023-05-01.
  - Action on repositories failing to meet these timelines will be determined in
    the future by the TOF.

## Rationale

Autotools is antiquated and cumbersome and its continued use is impacting
developer productivity. Autotools support leverages [`autoconf-archive`][4] to
detect available C++ standards but C++20 support took 1.5 years after the
standard was voted for acceptance before `autoconf-archive` [supported it][5].
There is also very limited support for building these packages outside of an
OE-SDK environment.

CMake is a more modern build system but is similarly lacking a widely applied
method for building outside of an OE-SDK environment (`ExternalProject` has been
used to some success by some developers, but it is only partially enabled by 2
out of 5 CMake repos and none of them have fully enabled it).

Meson is both modern and easily adapted to utilize [meson subproject][6] wrap
files which allow obtaining and building all the necessary dependencies to build
and test a repository on a typical Linux development system. Meson also has
built-in support for features such as unit testing, code coverage, code
formatting, and static analysis.

Converging on meson will: _ Reduce the necessity of maintaining tooling for
building the 3 different systems in CI. _ Consolidate build system expertise
into a single system and approach. _ Improve developer productivity by enabling
development activities outside of Yocto and OE-SDK environments. _ Enable better
alignment on and adoption of build system best practices. \* Enable rapid
adaptation of newer C++ standards.

---

[1]: https://github.com/mesonbuild/meson
[2]: https://gist.github.com/williamspatrick/b5fd618ef597cd6ad6dd97a633d5b264
[3]: https://github.com/openbmc/openbmc
[4]: https://github.com/autoconf-archive/autoconf-archive
[5]:
  http://git.savannah.gnu.org/gitweb/?p=autoconf-archive.git;a=commit;h=3311b6bdeff883c6a13952594a9dcb60bce6ba80
[6]: https://www.stwcx.xyz/blog/2021/04/18/meson-subprojects.html
