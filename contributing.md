Contributing to OpenBMC
=======================

Here's a little guide to working on OpenBMC. This document will always be
a work-in-progress, feel free to propose changes.

Authors:

  Jeremy Kerr <jk@ozlabs.org>

Structure
---------

OpenBMC has quite a modular structure, consisting of small daemons with a
limited set of responsibilities. These communicate over D-Bus with other
components, to implement the complete BMC system.

The BMC's interfaces to the external world are typically through a separate
daemon, which then translates those requests to D-Bus messages.

These separate projects are then compiled into the final system by the
overall 'openbmc' build infrastructure.

For future development, keep this design in mind. Components' functionality
should be logically grouped, and keep related parts together where it
makes sense.

Starting out
------------

If you're starting out with OpenBMC, you may want to take a look at the issues
tagged with 'bitesize'. These are fixes or enhancements that don't require
extensive knowledge of the OpenBMC codebase, and are easier for a newcomer to
start working with.

Check out that list here:

 https://github.com/issues?utf8=%E2%9C%93&q=is%3Aopen+is%3Aissue+user%3Aopenbmc+label%3Abitesize

If you need further details on any of these issues, feel free to add comments.

Coding style
------------

Components of the OpenBMC sources should have a consistent style.  If the source is
coming from another project, we choose to follow the existing style of the
upstream project.  Otherwise, conventions are chosen based on the language.

### Python

Python source should all conform to PEP8.  This style is well established
within the Python community and can be verified with the 'pep8' tool.

https://www.python.org/dev/peps/pep-0008/

### C

For C code, we typically use the Linux coding style, which is documented at:

  http://git.kernel.org/cgit/linux/kernel/git/torvalds/linux.git/tree/Documentation/process/submitting-patches.rst

In short:

  - Indent with tabs instead of spaces, set at 8 columns

  - 80-column lines

  - Opening braces on the end of a line, except for functions

This style can mostly be verified with 'astyle' as follows:

    astyle --style=linux --indent=tab=8 --indent=force-tab=8

### C++

See [C++ Style and Conventions](./cpp-style-and-conventions.md).

Submitting changes
------------------

We use git for almost everything. Changes should be sent as patches to their
relevant source tree - or a git pull request for convenience.

Each commit should be a self-contained logical unit, and smaller patches are
usually better. However, if there is no clear division, a larger patch is
okay. During development, it can be useful to consider how your change can be
submitted as logical units.

Commit messages are important. They should describe why the change is needed,
and what effects it will have on the system. Do not describe the actual
code change made by the patch; that's what the patch itself is for.

Use your full name for contributions, and include a "Signed-off-by" line,
to indicate that you agree to the Developer's Certificate of Origin (see below).

Ensure that your patch doesn't change unrelated areas. Be careful of
accidental whitespace changes - this makes review unnecessarily difficult.

The guidelines in the Linux kernel are very useful:

 http://git.kernel.org/cgit/linux/kernel/git/torvalds/linux.git/tree/Documentation/SubmittingPatches

Your contribution will generally need to be reviewed before being accepted.


Submitting changes via Gerrit server
------------------------------------

The OpenBMC Gerrit server supports GitHub credentials, its link is:

  https://gerrit.openbmc-project.xyz/#/q/status:open

_One time setup_: Login to the WebUI with your GitHub credentials and verify on
your account Settings that your SSH keys were imported:

  https://gerrit.openbmc-project.xyz/#/settings/

Most repositories are supported by the Gerrit server, the current list can be
found under Projects -> List:

  https://gerrit.openbmc-project.xyz/#/admin/projects/

If you're going to be working with Gerrit often, it's useful to create an SSH
host block in ~/.ssh/config. Ex:
```
Host openbmc.gerrit
        Hostname gerrit.openbmc-project.xyz
        Port 29418
        User your_github_id
```

From your OpenBMC git repository, add a remote to the Gerrit server, where
'openbmc_repo' is the current git repository you're working on, such as
phosphor-rest-server, and 'openbmc.gerrit' is the name of the Host previously added:

  `git remote add gerrit ssh://openbmc.gerrit/openbmc/openbmc_repo`

Obtain the git hook commit-msg to automatically add a Change-Id to the commit
message, which is needed by Gerrit:

  `gitdir=$(git rev-parse --git-dir)`

  `scp -p -P 29418 openbmc.gerrit:hooks/commit-msg ${gitdir}/hooks`

To submit a change set, commit your changes, and push to the Gerrit server,
where 'gerrit' is the name of the remote added with the git remote add command:

  `git push gerrit HEAD:refs/for/master`


Avoid references to non-public resources
----------------------------------------

Code and commit messages should not refer to companies' internal documents
or systems (including bug trackers). Other developers may not have access to
these, making future maintenance difficult.


Best practices for D-Bus interfaces
----------------------------------

 * New D-Bus interfaces should be reusable

 * Type signatures should and use the simplest types possible, appropriate
   for the data passed. For example, don't pass numbers as ASCII strings.

 * New D-Bus interfaces should be documented in the 'docs' repository, at:

     https://github.com/openbmc/docs/blob/master/dbus-interfaces.md

See also: http://dbus.freedesktop.org/doc/dbus-api-design.html


Best practices for C
--------------------

There are numerous resources available elsewhere, but a few items that are
relevant to OpenBMC work:

 * You almost never need to use `system(<some shell pipeline>)`. Reading and
   writing from files should be done with the appropriate system calls.

   Generally, it's much better to use `fork(); execve()` if you do need to
   spawn a process, especially if you need to provide variable arguments.

 * Use the `stdint` types (eg, `uint32_t`, `int8_t`) for data that needs to be
   a certain size. Use the `PRIxx` macros for printf, if necessary.

 * Don't assume that `char` is signed or unsigned.

 * Beware of endian considerations when reading to & writing from
   C types

 * Declare internal-only functions as `static`, declare read-only data
   as `const` where possible.

 * Ensure that your code compiles without warnings, especially for changes
   to the kernel.


Developer's Certificate of Origin 1.1
-------------------------------------

    By making a contribution to this project, I certify that:

    (a) The contribution was created in whole or in part by me and I
        have the right to submit it under the open source license
        indicated in the file; or

    (b) The contribution is based upon previous work that, to the best
        of my knowledge, is covered under an appropriate open source
        license and I have the right under that license to submit that
        work with modifications, whether created in whole or in part
        by me, under the same open source license (unless I am
        permitted to submit under a different license), as indicated
        in the file; or

    (c) The contribution was provided directly to me by some other
        person who certified (a), (b) or (c) and I have not modified
        it.

    (d) I understand and agree that this project and the contribution
        are public and that a record of the contribution (including all
        personal information I submit with it, including my sign-off) is
        maintained indefinitely and may be redistributed consistent with
        this project or the open source license(s) involved.
