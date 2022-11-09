Contributing to OpenBMC
=======================

First off, thanks for taking the time to contribute! The following is a set of
guidelines for contributing to an OpenBMC repository which are hosted in the
[OpenBMC Organization](https://github.com/openbmc) on GitHub. Feel free to
propose changes to this document.

## Code of Conduct

We enthusiastically adhere to our
[Code of Conduct](https://github.com/openbmc/docs/blob/master/code-of-conduct.md).
If you have any concerns, please check that document for guidelines on who can
help you resolve them.

## Inclusive Naming

OpenBMC relies on the Open Compute Project to provide guidelines on inclusive
naming.  The OCP guidelines can be found here:

https://www.opencompute.org/documents/ocp-terminology-guidelines-for-inclusion-and-openness

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

Before you make a contribution, execute one of the OpenBMC Contributor License
Agreements, _One time only_:

* [Individual CLA](https://drive.google.com/file/d/1k3fc7JPgzKdItEfyIoLxMCVbPUhTwooY)
* [Corporate CLA](https://drive.google.com/file/d/1d-2M8ng_Dl2j1odsvZ8o1QHAdHB-pNSH)

If you work for someone, consider asking them to execute the corporate CLA.
This allows other contributors that work for your employer to skip the CLA
signing process, they can just be added to the existing CCLA Schedule A.

After signing a CLA, send it to manager@lfprojects.org.

If you're looking for a place to get started with OpenBMC, you may want to take
a look at the issues tagged with 'bitesize'. These are fixes or enhancements
that don't require extensive knowledge of the OpenBMC codebase, and are easier
for a newcomer to start working with.

Check out that list here:

https://github.com/issues?utf8=%E2%9C%93&q=is%3Aopen+is%3Aissue+user%3Aopenbmc+label%3Abitesize

If you need further details on any of these issues, feel free to add comments.

Performing code reviews is another good way to get started.  Go to
https://gerrit.openbmc.org and click on the "all" and "open"
menu items, or if you are interested in a particular repository - for
example, "bmcweb" - type "status:open project:openbmc/bmcweb" into the
search bar and press the "search" button.  Then select a review.
Remember to be positive and add value with every review comment.

Coding style
------------

Components of the OpenBMC sources should have a consistent style.  If the
source is coming from another project, we choose to follow the existing style
of the upstream project.  Otherwise, conventions are chosen based on the
language.

### Python

Python source should all conform to PEP8.  This style is well established
within the Python community and can be verified with the 'pycodestyle' tool.

https://www.python.org/dev/peps/pep-0008/

### Python Formatting

If a repository has a setup.cfg file present in its root directory,
then CI will automatically verify the Python code meets the [pycodestyle](http://pycodestyle.pycqa.org/en/latest/intro.html)
requirements. This enforces PEP 8 standards on all Python code.

OpenBMC standards for Python match with PEP 8 so in general, a blank setup.cfg
file is all that's needed. If so desired, enforcement of 80
(vs. the default 79) chars is fine as well:
```
[pycodestyle]
max-line-length = 80
```
By default, pycodestyle does not enforce the following rules: E121, E123, E126,
E133, E226, E241, E242, E704, W503, and W504. These rules are ignored because
they are not unanimously accepted and PEP 8 does not enforce them. It is at the
repository maintainer's discretion as to whether to enforce the aforementioned
rules. These rules can be enforced by adding the following to the setup.cfg:
```
[pycodestyle]
ignore= NONE
```

### JavaScript

We follow the
[Google JavaScript Style Guide](https://google.github.io/styleguide/jsguide.html).

[Example .clang-format](https://www.github.com/openbmc/docs/blob/master/style/javascript/.clang-format)

### HTML/CSS

We follow the
[Google HTML/CSS Style Guide](https://google.github.io/styleguide/htmlcssguide.html).

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

Planning changes
----------------

If you are making a nontrivial change, you should plan how to
introduce it to the OpenBMC development community.

If you are planning a new function, you should get agreement that your
change will be accepted.  As early as you can, introduce the change
via the OpenBMC Discord server or email list to start
the discussion.  You may find a better way to do what you need.

If the feedback seems favorable or requests more details, continue
by submitting the design to gerrit starting with a copy of the
[design_template](https://github.com/openbmc/docs/blob/master/designs/design-template.md).

Submitting changes
------------------

We use git for almost everything. Most projects in OpenBMC use Gerrit to review
patches. Changes to an upstream project (e.g. Yocto Project, systemd, or the
Linux kernel) might need to be sent as patches or a git pull request.

### Organizing Commits

A good commit does exactly one thing. We prefer many small, atomic commits to
one large commit which makes many functional changes.
 - Too large: "convert foo to new API; also fix CVE 1234 in bar"
 - Too small: "move abc.h to top of include list" and "move xyz.h to bottom of
   include list"
 - Just right: "convert foo to new API" and "convert foo from tab to space"

Other commit tips:
 - If your change has a specification, sketch out your ideas first
   and work to get that accepted before completing the details.
 - Work at most a few days before seeking review.
 - Commit and review header files before writing code.
 - Commit and review each implementation module separately.

Often, creating small commits this way results in many commits that are
dependent on prior commits; Gerrit handles this situation well, so feel free to
push commits which are based on your change still in review. However, when
possible, your commit should stand alone on top of master - "Fix whitespace in
bar()" does not need to depend on "refactor foo()". Said differently, ensure
that topics that are not related to each other semantically are also not
related to each other in Git until they are merged into master.

When pushing a stack of patches, these commits will show up with that same
relationship in Gerrit. This means that each patch must be merged in order of
that relationship. So if one of the patches in the middle needs to be changed,
all the patches from that point on would need to be pushed to maintain the
relationship. This will effectively rebase the unchanged patches, which would
in turn trigger a new CI build. Ideally, changes from the entire patchset could
be done all in one go to reduce unnecessary rebasing.

When someone makes a comment on your commit in Gerrit, modify that commit and
send it again to Gerrit. This typically involves `git rebase --interactive` or
`git commit --amend`, for which there are many guides online. As mentioned in
the paragraph above, when possible you should make changes to multiple patches
in the same stack before you push, in order to minimize CI and notification
churn from the rebase operations.

Commits which include changes that can be tested by a unit test should also
include a unit test to exercise that change, within the same commit. Unit tests
should be clearly written - even more so than production code, unit tests are
meant primarily to be read by humans - and should test both good and bad
behaviors. Refer to the
[testing documentation](https://github.com/openbmc/docs/blob/master/testing/local-ci-build.md)
for help writing tests, as well as best practices.

Ensure that your patch doesn't change unrelated areas. Be careful of
accidental whitespace changes - this makes review unnecessarily difficult.

### Formatting Commit Messages

Your commit message should explain:

 - Concisely, *what* change are you making? e.g. "libpldm: Add APIs to enable
   PLDM Requester" This first line of your commit message is the subject line.
 - Comprehensively, *why* are you making that change? In some cases, like a
   small refactor, the why is fairly obvious. But in cases like the inclusion of
   a new feature, you should explain why the feature is needed.
 - Concisely, *how* did you test? (see below).

Try to include the component you are changing at the front of your subject line;
this typically comes in the form of the class, module, handler, or directory you
are modifying. e.g. "apphandler: refactor foo to new API"

Commit messages should follow the 50/72 rule: the subject line should not exceed
50 characters and the body should not exceed 72 characters. This is common
practice in many projects which use Git.

Exceptions to this are allowed in the form of links, which can be represented in
the form of:

'''
This implements [1]

....

[1] https://openbmc.org/myverylongurl.
'''

All commit messages must include a "Signed-off-by" line, which indicates that
you the contributor have agreed to the Developer Certificate of Origin
(see below). This line must include the full name you commonly use, often a
given name and a family name or surname. (ok: Sam Samuelsson, Robert A.
Heinlein; not ok: xXthorXx, Sam, RAH)

### Testing

Each commit is expected to be tested. The expectation of testing may vary from
subproject to subproject, but will typically include running all applicable
automated tests and performing manual testing. Each commit should be tested
separately, even if they are submitted together (an exception is when commits
to different projects depend on each other).

Commit messages should include a "Tested" field describing how you tested the
code changes in the patch. Example:
```
    Tested: I ran unit tests with "make check" (added 2 new tests) and manually
    tested on Witherspoon that Foo daemon no longer crashes at boot.
```

If the change required manual testing, describe what you did and what happened;
if it used to do something else before your change, describe that too.  If the
change can be validated entirely by running unit tests, say so in the "Tested:"
field.

### Linux Kernel

The guidelines in the Linux kernel are very useful:

https://www.kernel.org/doc/html/latest/process/submitting-patches.html

https://www.kernel.org/doc/html/latest/process/submit-checklist.html

Your contribution will generally need to be reviewed before being accepted.


Submitting changes via Gerrit server to OpenBMC
-----------------------------------------------

The OpenBMC Gerrit server supports GitHub credentials, its link is:

  https://gerrit.openbmc.org/#/q/status:open

_One time setup_: Login to the WebUI with your GitHub credentials and verify on
your Account Settings that your SSH keys were imported:

  https://gerrit.openbmc.org/#/settings/

Most repositories are supported by the Gerrit server, the current list can be
found under Projects -> List:

  https://gerrit.openbmc.org/#/admin/projects/

If you're going to be working with Gerrit often, it's useful to create an SSH
host block in ~/.ssh/config. Ex:
```
Host openbmc.gerrit
        Hostname gerrit.openbmc.org
        Port 29418
        User your_github_id
```

From your OpenBMC git repository, add a remote to the Gerrit server, where
'openbmc_repo' is the current git repository you're working on, such as
phosphor-state-manager, and 'openbmc.gerrit' is the name of the Host previously
added:

  `git remote add gerrit ssh://openbmc.gerrit/openbmc/openbmc_repo`

Alternatively, you can encode all the data in the URL:

  `git remote add gerrit ssh://your_github_id@gerrit.openbmc.org:29418/openbmc/openbmc_repo`

Then add the default branch for pushes to this remote:

  `git config remote.gerrit.push HEAD:refs/for/master`

Gerrit uses [Change-Ids](https://gerrit-review.googlesource.com/Documentation/user-changeid.html) to identify commits that belong to the same
review.  Configure your git repository to automatically add a
Change-Id to your commit messages.  The steps are:

  `gitdir=$(git rev-parse --git-dir)`

  `scp -p -P 29418 openbmc.gerrit:hooks/commit-msg ${gitdir}/hooks`

To submit a change set, commit your changes, and push to the Gerrit server,
where 'gerrit' is the name of the remote added with the git remote add command:

  `git push gerrit`

Sometimes you need to specify a topic (especially when working on a
feature that spans across few projects) or (un)mark the change as
Work-in-Progress. For that refer to [Gerrit
documentation](https://gerrit-review.googlesource.com/Documentation/intro-user.html#topics).

Gerrit will show you the URL link to your review.  You can also find
your reviews from the [OpenBMC Gerrit server](https://gerrit.openbmc.org) search bar
or menu (All > Open, or My > Changes).

Invite reviewers to review your changes.  Each OpenBMC repository has
an `OWNERS` file that lists required reviewers who are subject matter
experts.  Those reviewers may add additional reviewers.  To add
reviewers from the Gerrit web page, click the "add reviewers" icon by
the list of reviewers.

You are expected to respond to all comments.  And remember to use the
"reply" button to publish your replies for others to see.

The review results in the proposed change being accepted, rejected for
rework, or abandoned.  When you re-work your change, remember to use
`git commit --amend` so Gerrit handles the update as a new patch of
the same review.

Each repository is governed by a small group of maintainers who are
leaders with expertise in their area.  They are responsible for
reviewing changes and maintaining the quality of the code.  You'll
need a maintainer to accept your change, and they will look to the
other reviewers for guidance.  When accepted, your change will merge
into the OpenBMC project.


References to non-public resources
----------------------------------------

Code and commit messages shall not refer to companies' internal documents
or systems (including bug trackers). Other developers may not have access to
these, making future maintenance difficult.

Code contributed to OpenBMC must build from the publicly available sources,
with no dependencies on non-public resources (URLs, repositories, etc).

Best practices for D-Bus interfaces
----------------------------------

 * New D-Bus interfaces should be reusable

 * Type signatures should and use the simplest types possible, appropriate
   for the data passed. For example, don't pass numbers as ASCII strings.

 * D-Bus interfaces are defined in the `phosphor-dbus-interfaces` repository at:

     https://github.com/openbmc/phosphor-dbus-interfaces

See: http://dbus.freedesktop.org/doc/dbus-api-design.html


Best practices for C
--------------------

There are numerous resources available elsewhere, but a few items that are
relevant to OpenBMC work:

 * Do not use `system(<some shell pipeline>)`. Reading and writing from
   files should be done with the appropriate system calls.

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

## Pace of Review

Contributors who are used to code reviews by their team internal to their own
company, or who are not used to code reviews at all, are sometimes surprised by
the pace of code reviews in open source projects. Try to keep in mind that those
reviewing your patch may be contributing to OpenBMC in a volunteer or
partial-time capacity, may be in a timezone far from your own, and may
have very deep review queues already of patches which have been waiting longer
than yours. Do everything you can to make it easy for the reviewer to review
your contribution.

If you feel your patch has been missed entirely, of course, it's
alright to email the maintainers (addresses available in OWNERS file) or
ping them on Discord - but a reasonable timeframe to do so is on the order of a
week, not on the order of hours.

The maintainers' job is to ensure that incoming patches are as correct and easy
to maintain as possible. Part of the nature of open source is attrition -
contributors can come and go easily - so maintainers tend not to put stock in
promises such as "I will add unit tests in a later patch" or "I will be
implementing this proposal by the end of next month." This often manifests as
reviews which may seem harsh or exacting; please keep in mind that the community
is trying to collaborate with you to build a patch that will benefit the project
on its own.

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
