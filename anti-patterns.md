# OpenBMC Anti-patterns

From [Wikipedia](https://en.wikipedia.org/wiki/Anti-pattern):

"An anti-pattern is a common response to a recurring problem that is usually
ineffective and risks being highly counterproductive."

The developers of OpenBMC do not get 100% of decisions right 100% of the time.
That, combined with the fact that software development is often an exercise in
copying and pasting, results in mistakes happening over and over again.

This page aims to document some of the anti-patterns that exist in OpenBMC to
ease the job of those reviewing code. If an anti-pattern is spotted, rather that
repeating the same explanations over and over, a link to this document can be
provided.

<!-- begin copy/paste on next line -->

## Anti-pattern template [one line description]

### Identification

(1 paragraph) Describe how to spot the anti-pattern.

### Description

(1 paragraph) Describe the negative effects of the anti-pattern.

### Background

(1 paragraph) Describe why the anti-pattern exists. If you don't know, try
running git blame and look at who wrote the code originally, and ask them on the
mailing list or in Discord what their original intent was, so it can be
documented here (and you may possibly discover it isn't as much of an
anti-pattern as you thought). If you are unable to determine why the
anti-pattern exists, put: "Unknown" here.

### Resolution

(1 paragraph) Describe the preferred way to solve the problem solved by the
anti-pattern and the positive effects of solving it in the manner described.

<!-- end copy/paste on previous line -->

## Custom ArgumentParser object

### Identification

The ArgumentParser object is typically present to wrap calls to get options. It
abstracts away the parsing and provides a `[]` operator to access the
parameters.

### Description

Writing a custom ArgumentParser object creates nearly duplicate code in a
repository. The ArgumentParser itself is the same, however, the options provided
differ. Writing a custom argument parser re-invents the wheel on c++ command
line argument parsing.

### Background

The ArgumentParser exists because it was in place early and then copied into
each new repository as an easy way to handle argument parsing.

### Resolution

The CLI11 library was designed and implemented specifically to support modern
argument parsing. It handles the cases seen in OpenBMC daemons and has some
handy built-in validators, and allows easy customizations to validation.

## Explicit listing of shared library packages in RDEPENDS in bitbake metadata

### Identification

```bitbake
RDEPENDS_${PN} = "libsystemd"
```

### Description

Out of the box bitbake examines built applications, automatically adds runtime
dependencies and thus ensures any library packages dependencies are
automatically added to images, sdks, etc. There is no need to list them
explicitly in a recipe.

Dependencies change over time, and listing them explicitly is likely prone to
errors - the net effect being unnecessary shared library packages being
installed into images.

Consult [Yocto Project Mega-Manual][mega-manual-rdepends] for information on
when to use explicit runtime dependencies.

[mega-manual-rdepends]:
  https://www.yoctoproject.org/docs/latest/mega-manual/mega-manual.html#var-RDEPENDS

### Background

The initial bitbake metadata author for OpenBMC was not aware that bitbake added
these dependencies automatically. Initial bitbake metadata therefore listed
shared library dependencies explicitly, and was subsequently copy pasted.

### Resolution

Do not list shared library packages in RDEPENDS. This eliminates the possibility
of installing unnecessary shared library packages due to unmaintained library
dependency lists in bitbake metadata.

## Use of /usr/bin/env in systemd service files

### Identification

In systemd unit files:

```systemd
[Service]

ExecStart=/usr/bin/env some-application
```

### Description

Outside of OpenBMC, most applications that provide systemd unit files don't
launch applications in this way. So if nothing else, this just looks strange and
violates the
[princple of least astonishment](https://en.wikipedia.org/wiki/Principle_of_least_astonishment).

### Background

This anti-pattern exists because a requirement exists to enable live patching of
applications on read-only filesystems. Launching applications in this way was
part of the implementation that satisfied the live patch requirement. For
example:

```bash
/usr/bin/phosphor-hwmon
```

on a read-only filesystem becomes:

```bash
/usr/local/bin/phosphor-hwmon`
```

on a writeable /usr/local filesystem.

### Resolution

The /usr/bin/env method only enables live patching of applications. A method
that supports live patching of any file in the read-only filesystem has emerged.
Assuming a writeable filesystem exists _somewhere_ on the bmc, something like:

```bash
mkdir -p /var/persist/usr
mkdir -p /var/persist/work/usr
mount -t overlay -o lowerdir=/usr,upperdir=/var/persist/usr,workdir=/var/persist/work/usr overlay /usr
```

can enable live system patching without any additional requirements on how
applications are launched from systemd service files. This is the preferred
method for enabling live system patching as it allows OpenBMC developers to
write systemd service files in the same way as most other projects.

To undo existing instances of this anti-pattern remove /usr/bin/env from systemd
service files and replace with the fully qualified path to the application being
launched. For example, given an application in /usr/bin:

```bash
sed -i s,/usr/bin/env ,/usr/bin/, foo.service
```

## Incorrect placement of executables in /sbin, /usr/sbin or /bin, /usr/bin

### Identification

OpenBMC executables that are installed in `/usr/sbin`. `$sbindir` in bitbake
metadata, makefiles or configure scripts. systemd service files pointing to
`/bin` or `/usr/bin` executables.

### Description

Installing OpenBMC applications in incorrect locations violates the
[principle of least astonishment](https://en.wikipedia.org/wiki/Principle_of_least_astonishment)
and more importantly violates the
[FHS](https://en.wikipedia.org/wiki/Filesystem_Hierarchy_Standard).

### Background

There are typically two types of executables:

1. Long-running daemons started by, for instance, systemd service files and
   _NOT_ intended to be directly executed by users.
2. Utilities intended to be used by a user as a CLI.

Executables of type 1 should not be placed anywhere in `${PATH}` because it is
confusing and error-prone to users, but should instead be placed in a
`/usr/libexec/<package>` subdirectory.

Executables of type 2 should be placed in `/usr/bin` because they are intended
to be used by users and should be in `${PATH}` (also, `sbin` is inappropriate as
we transition to having non-root access).

The sbin anti-pattern exists because the FHS was misinterpreted at an early
point in OpenBMC project history, and has proliferated ever since.

From the hier(7) man page:

```bash
/usr/bin This is the primary directory for executable programs.  Most programs
executed by normal users which are not needed for booting or for repairing the
system and which are not installed locally should be placed in this directory.

/usr/sbin This directory contains program binaries for system administration
which are not essential for the boot process, for mounting /usr, or for system
repair.

/usr/libexec Directory  contains  binaries for internal use only and they are
not meant to be executed directly by users shell or scripts.
```

The FHS description for `/usr/sbin` refers to "system administration" but the
de-facto interpretation of the system being administered refers to the OS
installation and not a system in the OpenBMC sense of managed system. As such
OpenBMC applications should be installed in `/usr/bin`.

It is becoming common practice in Linux for daemons to now be moved to `libexec`
and considered "internal use" from the perspective of the systemd service file
that controls their launch.

### Resolution

Install OpenBMC applications in `/usr/libexec` or `/usr/bin/` as appropriate.

## Handling unexpected error codes and exceptions

### Identification

The anti-pattern is for an application to continue processing after it
encounters unexpected conditions in the form of error codes and exceptions and
not capturing the data needed to resolve the problem.

Example C++ code:

```cpp
using InternalFailure = sdbusplus::xyz::openbmc_project::Common::Error::InternalFailure;
try
{
  ... use d-Bus APIs...
}
catch (InternalFailure& e)
{
    phosphor::logging::commit<InternalFailure>();
}
```

### Description

Suppressing unexpected errors can lead an application to incorrect or erratic
behavior which can affect the service it provides and the overall system.

Writing a log entry instead of a core dump may not give enough information to
debug a truly unexpected problem, so developers do not get a chance to
investigate problems and the system's reliability does not improve over time.

### Background

Programmers want their application to continue processing when it encounters
conditions it can recover from. Sometimes they try too hard and continue when it
is not appropriate.

Programmers also want to log what is happening in the application, so they write
log entries that give debug data when something goes wrong, typically targeted
for their use. They don't consider how the log entry is consumed by the BMC
administrator or automated service tools.

The `InternalFailure` in the [Phosphor logging README][] is overused.

[phosphor logging readme]:
  https://github.com/openbmc/phosphor-logging/blob/master/README.md

### Resolution

Several items are needed:

1. Check all places where a return code or errno value is given. Strongly
   consider that a default error handler should throw an exception, for example
   `std::system_error`.
2. Have a good reason to handle specific error codes. See below.
3. Have a good reason to handle specific exceptions. Allow other exceptions to
   propagate.
4. Document (in terms of impacts to other services) what happens when this
   service fails, stops, or is restarted. Use that to inform the recovery
   strategy.

In the error handler:

1. Consider what data (if any) should be logged. Determine who will consume the
   log entry: BMC developers, administrator, or an analysis tool. Usually the
   answer is more than one of these.

   The following example log entries use an IPMI request to set network access
   on, but the user input is invalid.
   - BMC Developer: Reference internal applications, services, pids, etc. the
     developer would be familiar with.

     Example:
     `ipmid service successfully processed a network setting packet, however the user input of USB0 is not a valid network interface to configure.`

   - Administrator: Reference the external interfaces of the BMC such as the
     REST API. They can respond to feedback about invalid input, or a need to
     restart the BMC.

     Example:
     `The network interface of USB0 is not a valid option. Retry the IPMI command with a valid interface.`

   - Analyzer tool: Consider breaking the log down and including several
     properties which an analyzer can leverage. For instance, tagging the log
     with 'Internal' is not helpful. However, breaking that down into something
     like `[UserInput][IPMI][Network]` tells at a quick glance that the input
     received for configuring the network via an IPMI command was invalid.
     Categorization and system impact are key things to focus on when creating
     logs for an analysis application.

     Example:
     `[UserInput][IPMI][Network][Config][Warning] Interface USB0 not valid.`

2. Determine if the application can fully recover from the condition. If not,
   don't continue. Allow the system to determine if it writes a core dump or
   restarts the service. If there are severe impacts when the service fails,
   consider using a better error recovery mechanism.

## Non-standard debug application options and logging

### Identification

An application uses non-standard methods on startup to indicate verbose logging
and/or does not utilize standard systemd-journald debug levels for logging.

### Description

When debugging issues within OpenBMC that cross multiple applications, it's very
difficult to enable the appropriate debug when different applications have
different mechanisms for enabling debug. For example, different OpenBMC
applications take the following as command line parameters to enable extra
debug:

- 0xff, --vv, -vv, -v, --verbose, `<and more>`

Along these same lines, some applications then have their own internal methods
of writing debug data. They use std::cout, printf, fprintf, ... Doing this
causes other issues when trying to compare debug data across different
applications that may be having their buffers flushed at different times (and
picked up by journald).

### Background

Everyone has their own ways to debug. There was no real standard within OpenBMC
on how to do it so everyone came up with whatever they were familiar with.

### Resolution

If an OpenBMC application is to support enhanced debug via a command line then
it will support the standard "-v,--verbose" option.

In general, OpenBMC developers should utilize the "debug" journald level for
debug messages. This can be enabled/disabled globally and would apply to all
applications. If a developer believes this would cause too much debug data in
certain cases then they can protect these journald debug calls around a
--verbose command line option.

## DBus interface representing GPIOs

### Identification

Desire to expose a DBus interface to drive GPIOs, for example:

- <https://lore.kernel.org/openbmc/YV21cD3HOOGi7K2f@heinlein/>
- <https://lore.kernel.org/openbmc/CAH2-KxBV9_0Dt79Quy0f4HkXXPdHfBw9FsG=4KwdWXBYNEA-ew@mail.gmail.com/>
- <https://lore.kernel.org/openbmc/YtPrcDzaxXiM6vYJ@heinlein.stwcx.org.github.beta.tailscale.net/>

### Description

Platform functionality selected by GPIOs might equally be selected by other
means with a shift in system design philosophy. As such, GPIOs are a (hardware)
implementation detail. Exposing the implementation on DBus forces the
distribution of platform design details across multiple applications, which
violates the software design principle of [low coupling][coupling] and impacts
our confidence in maintenance.

[coupling]: https://en.wikipedia.org/wiki/Coupling_%28computer_programming%29

### Background

GPIOs are often used to select functionality that might be hard to generalise,
and therefore hard to abstract. If this is the case, then the functionality in
question is probably best wrapped up as an implementation detail of a behaviour
that is more generally applicable (e.g. host power-on procedures).

### Resolution

Consider what functionality the GPIO provides and design or exploit an existing
interface to expose that behaviour instead.

## Very long lambda callbacks

### Identification

C++ code that is similar to the following:

```cpp
dbus::utility::getSubTree("/", interfaces,
                         [asyncResp](boost::system::error_code& ec,
                                     MapperGetSubTreeResult& res){
                            <too many lines of code>
                         })
```

### Description

Inline lambdas, while useful in some contexts, are difficult to read, and have
inconsistent formatting with tools like `clang-format`, which causes significant
problems in review, and in applying patchsets that might have minor conflicts.
In addition, because they are declared in a function scope, they are difficult
to unit test, and produce log messages that are difficult to read given their
unnamed nature. They are also difficult to debug, lacking any symbol name to
which to attach breakpoints or tracepoints.

### Background

Lambdas are a natural approach to implementing callbacks in the context of
asynchronous IO. Further, the Boost and sdbusplus ASIO APIs tend to encourage
this approach. Doing something other than lambdas requires more effort, and so
these options tend not to be chosen without pressure to do so.

### Resolution

Prefer to either use `std::bind_front` and a method or static function to handle
the return, or a lambda that is less than 10 lines of code to handle an error
inline. In cases where `std::bind_front` cannot be used, such as in
`sdbusplus::asio::connection::async_method_call`, keep the lambda length less
than 10 lines, and call the appropriate function for handling non-trivial
transforms.

```cpp
void afterGetSubTree(std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                     boost::system::error_code& ec,
                     MapperGetSubTreeResult& res){
   <many lines of code>
}
dbus::utility::getSubTree("/xyz/openbmc_project/inventory", interfaces,
                         std::bind_front(afterGetSubTree, asyncResp));
```

See also the [Cpp Core Guidelines][] for generalized guidelines on when lambdas
are appropriate. The above recommendation is aligned with the Cpp Core
Guidelines.

[Cpp Core Guidelines]:
  https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#f11-use-an-unnamed-lambda-if-you-need-a-simple-function-object-in-one-place-only

## Placing internal headers in a parallel subtree

### Identification

Declaring types and functions that do not participate in a public API of a
library in header files that do not live alongside their implementation in the
source tree.

### Description

There's no reason to put internal headers in a parallel subtree (for example,
`include/`). It's more effort to organise, puts unnecessary distance between
declarations and definitions, and increases effort required in the build system
configuration.

### Background

In C and C++, header files expose the public API of a library to its consumers
and need to be installed onto the developer's system in a location known to the
compiler. To delineate which header files are to be installed and which are not,
the public header files are often placed into an `include/` subdirectory of the
source tree to mark their importance.

Any functions or structures that are implementation details of the library
should not be provided in its installed header files. Ignoring this philosphy
over-exposes the library's design and may lead to otherwise unnecessary API or
ABI breaks in the future.

Further, projects whose artifacts are only application binaries have no public
API or ABI in the sense of a library. Any header files in the source tree of
such projects have no need to be installed onto a developer's system and
segregation in path from the implementation serves no purpose.

### Resolution

Place internal header files immediately alongside source files containing the
corresponding implementation.

## Ill-defined data structuring in `lg2` message strings

### Identification

Attempts at encoding information into the journal's MESSAGE string that is at
most only plausible to parse using a regex while also reducing human
readability. For example:

```cpp
error(
    "Error getting time, PATH={BMC_TIME_PATH} TIME INTERFACE={TIME_INTF}  ERROR={ERR_EXCEP}",
    "BMC_TIME_PATH", bmcTimePath, "TIME_INTF", timeInterface,
    "ERR_EXCEP", e);
```

### Description

[`lg2` is OpenBMC's preferred C++ logging interface][phosphor-logging-lg2] and
is implemented on top of the systemd journal and its library APIs.
[systemd-journald provides structured logging][systemd-structured-logging],
which allows us to capture additional metadata alongside the provided message.

[phosphor-logging-lg2]:
  https://github.com/openbmc/phosphor-logging/blob/master/docs/structured-logging.md
[systemd-structured-logging]:
  https://0pointer.de/blog/projects/journal-submit.html

The concept of structured logging allows for convenient programmable access to
metadata associated with a log event. The journal captures a default set of
metadata with each message logged. However, the primary way the entries are
exposed to users is `journalctl`'s default behaviour of printing just the
`MESSAGE` value for each log entry. This may lead to a misunderstanding that the
printed message is the only way to expose related metadata for investigating
defects.

For human ergonomics `lg2` provides the ability to interpolate structured data
into the journal's `MESSAGE` string. This aids with human inspection of the logs
as it becomes possible to highlight important metadata for a log event. However,
it's not intended that this interpolation be used for ad-hoc, ill-defined
attempts at exposing metadata for automated analysis.

All key-value pairs provided to the `lg2` logging APIs are captured in the
structured log event, regardless of whether any particular key is interpolated
into the `MESSAGE` string. It is always possible to recover the information
associated with the log event even if it's not captured in the `MESSAGE` string.

`phosphor-time-manager` demonstrates a reasonable use of the `lg2` APIs. One
logging instance in the code [is as follows][phosphor-time-manager-lg2]:

[phosphor-time-manager-lg2]:
  https://github.com/openbmc/phosphor-time-manager/blob/5ce9ac0e56440312997b25771507585905e8b360/manager.cpp#L98

```cpp
info("Time mode has been changed to {MODE}", "MODE", newMode);
```

By default, this renders in the output of `journalctl` as:

```text
Sep 23 06:09:57 bmc phosphor-time-manager[373]: Time mode has been changed to xyz.openbmc_project.Time.Synchronization.Method.NTP
```

However, we can use some journalctl commandline options to inspect the
structured data associated with the log entry:

```bash
# journalctl --identifier=phosphor-time-manager --boot --output=verbose | grep -v '^    _' | head -n 9
Sat 2023-09-23 06:09:57.645208 UTC [s=85c1cb5f8e02445aa110a5164c9c07f6;i=244;b=ffd111d3cdca41c8893bb728a1c6cb20;m=133a5a0;t=606009314d0d9;x=9a54e8714754a6cb]
    PRIORITY=6
    MESSAGE=Time mode has been changed to xyz.openbmc_project.Time.Synchronization.Method.NTP
    LOG2_FMTMSG=Time mode has been changed to {MODE}
    CODE_FILE=/usr/src/debug/phosphor-time-manager/1.0+git/manager.cpp
    CODE_LINE=98
    CODE_FUNC=bool phosphor::time::Manager::setCurrentTimeMode(const std::string&)
    MODE=xyz.openbmc_project.Time.Synchronization.Method.NTP
    SYSLOG_IDENTIFIER=phosphor-time-manager
```

Here we find that `MODE` and its value are captured as its own metadata entry in
the structured data, as well as being interpolated into `MESSAGE` as requested.
Additionally, from the log entry we can find _how_ `MODE` was interpolated into
`MESSAGE` using the format string captured in the `LOG2_FMTMSG` metadata entry.

`LOG2_FMTMSG` also provides a stable handle for identifying the existence of a
specific class of log events in the journal, further aiding automated analysis.

### Background

A variety of patches such as [PLDM:Catching exception precisely and printing
it][openbmc-gerrit-67994] added a number of ad-hoc, ill-defined attempts at
providing all the metadata through the `MESSAGE` entry.

[openbmc-gerrit-67994]: https://gerrit.openbmc.org/c/openbmc/pldm/+/67994

### Resolution

`lg2` messages should be formatted for consumption by humans. They should not
contain ad-hoc, ill-defined attempts at integrating metadata for automated
analysis.
